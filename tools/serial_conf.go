// serial_conf.go
//
// Golang CLI tool for reading/writing OHS config fields over serial port.
// Uses MsgPack serialization.
//
// Usage:
//   go run serial_conf.go -port /dev/ttyUSB0 -read conf.group -index 0
//   go run serial_conf.go -port /dev/ttyUSB0 -write conf.armDelay -value 10
//   go run serial_conf.go -port /dev/ttyUSB0 -write conf.mqtt -json '{"adr":"mqtt.local","usr":"admin","pwd":"pass","prt":1883,"set":1}'
//
// Dependencies:
//   go get go.bug.st/serial
//   go get github.com/vmihailenco/msgpack/v5

package main

import (
	"encoding/binary"
	"encoding/json"
	"flag"
	"fmt"
	"log"
	"os"
	"strconv"
	"time"

	"github.com/vmihailenco/msgpack/v5"
	"go.bug.st/serial"
)

const (
	STX         = 0x02
	CmdRead     = 0x01
	CmdReadRsp  = 0x02
	CmdWrite    = 0x03
	CmdWriteRsp = 0x04
)

// crc16CCITT computes CRC16-CCITT over data
func crc16CCITT(data []byte) uint16 {
	crc := uint16(0xFFFF)
	for _, b := range data {
		crc ^= uint16(b) << 8
		for j := 0; j < 8; j++ {
			if crc&0x8000 != 0 {
				crc = (crc << 1) ^ 0x1021
			} else {
				crc <<= 1
			}
		}
	}
	return crc
}

// buildFrame creates a protocol frame: STX + CMD + LEN(2B LE) + payload + CRC16(2B LE)
func buildFrame(cmd byte, payload []byte) []byte {
	frame := make([]byte, 0, 4+len(payload)+2)
	frame = append(frame, STX, cmd)
	lenBuf := make([]byte, 2)
	binary.LittleEndian.PutUint16(lenBuf, uint16(len(payload)))
	frame = append(frame, lenBuf...)
	frame = append(frame, payload...)

	// CRC over CMD + payload
	crcData := make([]byte, 0, 1+len(payload))
	crcData = append(crcData, cmd)
	crcData = append(crcData, payload...)
	crc := crc16CCITT(crcData)
	crcBuf := make([]byte, 2)
	binary.LittleEndian.PutUint16(crcBuf, crc)
	frame = append(frame, crcBuf...)

	return frame
}

// readFrame reads a response frame from serial port
func readFrame(port serial.Port) (cmd byte, payload []byte, err error) {
	// Read STX
	buf := make([]byte, 1)
	deadline := time.Now().Add(3 * time.Second)
	for {
		if time.Now().After(deadline) {
			return 0, nil, fmt.Errorf("timeout waiting for STX")
		}
		n, err := port.Read(buf)
		if err != nil {
			return 0, nil, err
		}
		if n > 0 && buf[0] == STX {
			break
		}
	}

	// Read CMD
	if _, err := readFull(port, buf, 1); err != nil {
		return 0, nil, fmt.Errorf("reading CMD: %w", err)
	}
	cmd = buf[0]

	// Read LEN (2 bytes LE)
	lenBuf := make([]byte, 2)
	if _, err := readFull(port, lenBuf, 2); err != nil {
		return 0, nil, fmt.Errorf("reading LEN: %w", err)
	}
	payloadLen := binary.LittleEndian.Uint16(lenBuf)

	// Read payload
	payload = make([]byte, payloadLen)
	if payloadLen > 0 {
		if _, err := readFull(port, payload, int(payloadLen)); err != nil {
			return 0, nil, fmt.Errorf("reading payload: %w", err)
		}
	}

	// Read CRC16
	crcBuf := make([]byte, 2)
	if _, err := readFull(port, crcBuf, 2); err != nil {
		return 0, nil, fmt.Errorf("reading CRC: %w", err)
	}
	rxCrc := binary.LittleEndian.Uint16(crcBuf)

	// Verify CRC
	crcData := make([]byte, 0, 1+len(payload))
	crcData = append(crcData, cmd)
	crcData = append(crcData, payload...)
	calcCrc := crc16CCITT(crcData)
	if rxCrc != calcCrc {
		return 0, nil, fmt.Errorf("CRC mismatch: got %04x, expected %04x", rxCrc, calcCrc)
	}

	return cmd, payload, nil
}

// readFull reads exactly n bytes from serial port with timeout
func readFull(port serial.Port, buf []byte, n int) (int, error) {
	total := 0
	deadline := time.Now().Add(3 * time.Second)
	for total < n {
		if time.Now().After(deadline) {
			return total, fmt.Errorf("timeout after %d/%d bytes", total, n)
		}
		read, err := port.Read(buf[total:n])
		if err != nil {
			return total, err
		}
		total += read
	}
	return total, nil
}

func main() {
	portName := flag.String("port", "/dev/ttyUSB0", "Serial port")
	baud := flag.Int("baud", 115200, "Baud rate")
	readField := flag.String("read", "", "Field name to read (e.g. conf.group)")
	writeField := flag.String("write", "", "Field name to write")
	index := flag.Int("index", 0, "Array index (for array fields)")
	value := flag.String("value", "", "Value to write (for simple types)")
	jsonVal := flag.String("json", "", "JSON value to write (for struct types)")
	flag.Parse()

	if *readField == "" && *writeField == "" {
		fmt.Fprintf(os.Stderr, "Usage: specify -read or -write\n")
		flag.PrintDefaults()
		os.Exit(1)
	}

	// Open serial port
	mode := &serial.Mode{
		BaudRate: *baud,
		DataBits: 8,
		StopBits: serial.OneStopBit,
		Parity:   serial.NoParity,
	}
	port, err := serial.Open(*portName, mode)
	if err != nil {
		log.Fatalf("Failed to open serial port: %v", err)
	}
	defer port.Close()
	port.SetReadTimeout(100 * time.Millisecond)

	if *readField != "" {
		doRead(port, *readField, uint8(*index))
	} else {
		doWrite(port, *writeField, uint8(*index), *value, *jsonVal)
	}
}

func doRead(port serial.Port, field string, index uint8) {
	// Build MsgPack payload: { "f": field, "i": index }
	payload, err := msgpack.Marshal(map[string]interface{}{
		"f": field,
		"i": index,
	})
	if err != nil {
		log.Fatalf("MsgPack encode error: %v", err)
	}

	frame := buildFrame(CmdRead, payload)
	fmt.Printf(">> READ %s[%d]\n", field, index)

	_, err = port.Write(frame)
	if err != nil {
		log.Fatalf("Write error: %v", err)
	}

	// Read response
	cmd, rspPayload, err := readFrame(port)
	if err != nil {
		log.Fatalf("Read response error: %v", err)
	}

	if cmd == CmdReadRsp {
		var result map[string]interface{}
		if err := msgpack.Unmarshal(rspPayload, &result); err != nil {
			log.Fatalf("MsgPack decode error: %v", err)
		}
		prettyJSON, _ := json.MarshalIndent(result, "", "  ")
		fmt.Printf("<< Response:\n%s\n", string(prettyJSON))
	} else if cmd == CmdWriteRsp {
		var result map[string]interface{}
		msgpack.Unmarshal(rspPayload, &result)
		fmt.Printf("<< Error response: %v\n", result)
	} else {
		fmt.Printf("<< Unexpected CMD: 0x%02x\n", cmd)
	}
}

func doWrite(port serial.Port, field string, index uint8, value string, jsonVal string) {
	var data interface{}

	if jsonVal != "" {
		// Parse JSON for struct types
		if err := json.Unmarshal([]byte(jsonVal), &data); err != nil {
			log.Fatalf("JSON parse error: %v", err)
		}
	} else if value != "" {
		// Try parsing as number first
		if v, err := strconv.ParseUint(value, 10, 32); err == nil {
			data = v
		} else if v, err := strconv.ParseInt(value, 10, 16); err == nil {
			data = v
		} else if v, err := strconv.ParseFloat(value, 32); err == nil {
			data = float32(v)
		} else {
			data = value // string
		}
	} else {
		log.Fatal("Specify -value or -json for write operation")
	}

	// Build MsgPack payload: { "f": field, "i": index, "d": data }
	payload, err := msgpack.Marshal(map[string]interface{}{
		"f": field,
		"i": index,
		"d": data,
	})
	if err != nil {
		log.Fatalf("MsgPack encode error: %v", err)
	}

	frame := buildFrame(CmdWrite, payload)
	fmt.Printf(">> WRITE %s[%d] = %v\n", field, index, data)

	_, err = port.Write(frame)
	if err != nil {
		log.Fatalf("Write error: %v", err)
	}

	// Read response
	cmd, rspPayload, err := readFrame(port)
	if err != nil {
		log.Fatalf("Read response error: %v", err)
	}

	if cmd == CmdWriteRsp {
		var result map[string]interface{}
		if err := msgpack.Unmarshal(rspPayload, &result); err != nil {
			log.Fatalf("MsgPack decode error: %v", err)
		}
		status, ok := result["s"]
		if ok && fmt.Sprintf("%v", status) == "1" {
			fmt.Println("<< Write OK")
		} else {
			fmt.Printf("<< Write FAILED: %v\n", result)
		}
	} else {
		fmt.Printf("<< Unexpected CMD: 0x%02x\n", cmd)
	}
}
