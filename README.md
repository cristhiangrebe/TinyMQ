# TinyMQ - Lightweight MQTT Broker in C

A minimal, high-performance MQTT v3.1.1 broker implementation written from scratch in C. Designed for educational purposes and resource-constrained environments.

## 🎯 Project Goals

- **Educational**: Learn MQTT protocol internals by implementing from scratch
- **Minimal**: Keep codebase small and readable for easy understanding
- **Performance**: Efficient memory usage and fast packet processing
- **Standards Compliant**: Full MQTT v3.1.1 specification support
- **Portable**: Pure C implementation with minimal dependencies

## 🚀 Features

### Currently Implemented
- ✅ MQTT packet parsing and serialization
- ✅ Complete packet structure definitions
- ✅ Variable length encoding/decoding
- ✅ Support for all major packet types:
  - CONNECT/CONNACK
  - PUBLISH/PUBACK/PUBREC/PUBREL/PUBCOMP
  - SUBSCRIBE/SUBACK/UNSUBSCRIBE/UNSUBACK
  - PINGREQ/PINGRESP
  - DISCONNECT

### Roadmap
- 🔄 **In Progress**: Core broker functionality
- ⏳ **Planned**: Client connection management
- ⏳ **Planned**: Topic subscription handling
- ⏳ **Planned**: QoS levels implementation (0, 1, 2)
- ⏳ **Planned**: Retained messages
- ⏳ **Planned**: Last Will and Testament
- ⏳ **Planned**: Authentication and authorization
- ⏳ **Planned**: Configuration system
- ⏳ **Planned**: Logging and monitoring

## 🏗️ Architecture

```
├── src/
│   ├── mqtt.h          # MQTT protocol definitions and structures
│   ├── mqtt.c          # Packet parsing and protocol logic
│   ├── pack.h          # Binary data serialization utilities
│   └── pack.c          # Packing/unpacking implementations
├── tutorial/           # Learning materials and examples
└── CMakeLists.txt      # Build configuration
```

## 🛠️ Building

### Prerequisites
- GCC or Clang compiler
- CMake 3.10+
- Standard C library

### Compile
```bash
# Using CMake (recommended)
mkdir build && cd build
cmake ..
make

# Or direct compilation
gcc -Wall -Wextra -I./src src/*.c -o tinymq
```

## 🧪 Testing

```bash
# Compile test program
gcc -Wall -Wextra -I./src src/mqtt.c src/pack.c test/test_packets.c -o test_runner

# Run tests
./test_runner
```

## 📚 Learning Resources

The `tutorial/` directory contains step-by-step explanations of:
- MQTT protocol basics
- Packet structure analysis
- Implementation details
- Performance considerations

## 🤝 Contributing

This is primarily an educational project, but contributions are welcome!

1. Fork the repository
2. Create a feature branch
3. Follow the existing code style
4. Add tests for new functionality
5. Submit a pull request

### Code Style
- Use 4-space indentation
- Keep functions small and focused
- Add comments for complex protocol logic
- Follow MQTT specification terminology

## 📖 MQTT Protocol Reference

This implementation follows [MQTT Version 3.1.1](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html) specification.

Key concepts:
- **QoS Levels**: At most once (0), at least once (1), exactly once (2)
- **Topics**: Hierarchical publish/subscribe routing
- **Retained Messages**: Last message on topic stored by broker
- **Clean Session**: Connection state persistence control
- **Keep Alive**: Connection health monitoring

## 🎓 Educational Goals

This project demonstrates:
- Network protocol implementation
- Binary data parsing and serialization
- Memory management in C
- Event-driven programming patterns
- Client-server architecture design

## 📄 License

MIT License - See [LICENSE](LICENSE) file for details.

## 🔗 References

- [MQTT v3.1.1 Specification](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html)
- [MQTT.org](https://mqtt.org/)
- [Eclipse Mosquitto](https://mosquitto.org/) - Reference implementation

---

**Status**: 🚧 Active Development | **Version**: 0.1.0-alpha

*Building MQTT from the ground up, one packet at a time.*
