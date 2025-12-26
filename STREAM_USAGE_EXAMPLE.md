# SPFS ReadOnlyFile Stream Interface Usage

The `ReadOnlyFile` class now supports reading via standard C++ stream operators through the `getInputStream()` method.

## Usage Examples

### Example 1: Reading line by line
```cpp
auto file = directory->openFile("example.txt");
auto stream = file->getInputStream();

std::string line;
while (std::getline(*stream, line)) {
    // Process each line
    std::cout << line << std::endl;
}
```

### Example 2: Reading formatted data
```cpp
auto file = directory->openFile("data.txt");
auto stream = file->getInputStream();

int value;
std::string text;
*stream >> value >> text;
```

### Example 3: Reading character by character
```cpp
auto file = directory->openFile("text.txt");
auto stream = file->getInputStream();

char ch;
while (stream->get(ch)) {
    // Process each character
    std::cout << ch;
}
```

### Example 4: Reading with seeking
```cpp
auto file = directory->openFile("binary.dat");
auto stream = file->getInputStream();

// Seek to position 100
stream->seekg(100);

// Read 4 bytes
uint32_t value;
stream->read(reinterpret_cast<char*>(&value), sizeof(value));

// Seek relative to current position
stream->seekg(10, std::ios::cur);

// Seek from end
stream->seekg(-20, std::ios::end);
```

### Example 5: Reading entire content into string
```cpp
auto file = directory->openFile("document.txt");
auto stream = file->getInputStream();

std::stringstream buffer;
buffer << stream->rdbuf();
std::string content = buffer.str();
```

### Example 6: Checking stream state
```cpp
auto file = directory->openFile("data.bin");
auto stream = file->getInputStream();

if (stream->good()) {
    // Stream is ready
}

if (stream->eof()) {
    // End of file reached
}

if (stream->fail()) {
    // Operation failed
}
```

## Implementation Details

The stream interface is implemented using:
- **`ReadOnlyFileStreamBuf`**: A custom `std::streambuf` implementation that wraps the memory-mapped file data
- **`getInputStream()`**: Returns a `std::unique_ptr<std::istream>` for reading the file

### Features:
- ✅ Full standard C++ stream operator support (`>>`, `getline()`, `read()`, etc.)
- ✅ Seeking support (forward, backward, absolute, relative)
- ✅ Zero-copy access to file data (memory-mapped)
- ✅ Automatic memory management via unique_ptr
- ✅ Compatible with all standard library stream algorithms

### Benefits:
- Use familiar C++ stream syntax
- Integrate seamlessly with existing C++ code expecting streams
- Supports all stream manipulators (e.g., `std::hex`, `std::fixed`)
- Can be passed to functions accepting `std::istream&`

## Comparison with Existing Methods

| Method | Use Case | Return Type |
|--------|----------|-------------|
| `readAsString()` | Read entire file as string | `std::string` |
| `readAsVector()` | Read entire file as bytes | `std::vector<uint8_t>` |
| `readBytes(offset, size)` | Read specific portion | `std::vector<uint8_t>` |
| `getInputStream()` | Stream-based reading | `std::unique_ptr<std::istream>` |

Choose the appropriate method based on your needs:
- Use existing methods for simple, one-shot reads of the entire file or portions
- Use `getInputStream()` for sequential reading, parsing, or when you need stream semantics
