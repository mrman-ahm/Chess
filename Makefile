# Compiler and flags
CXX = g++
CXXFLAGS = -Iinclude -Iexternal/SFML/include -Wall -Wextra -Wpedantic -O2 -DNDEBUG -std=c++17
LDFLAGS = -Lexternal/SFML/lib -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-system

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/sixty-four.exe
TEST_TARGET = $(BIN_DIR)/chess-tests.exe
PACKAGE_DIR = dist/Sixty-Four

# Rules
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@if not exist $(BIN_DIR) mkdir $(BIN_DIR)
	@copy /Y "external\SFML\bin\*.dll" "$(BIN_DIR)\" >nul
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@if exist $(OBJ_DIR) rmdir /s /q $(OBJ_DIR)
	@if exist $(BIN_DIR) rmdir /s /q $(BIN_DIR)

run: all
	$(TARGET)

test:
	@if not exist $(BIN_DIR) mkdir $(BIN_DIR)
	$(CXX) $(CXXFLAGS) tests/ChessTests.cpp src/Chess.cpp -o $(TEST_TARGET)
	$(TEST_TARGET)

package: all
	@if exist "$(PACKAGE_DIR)" rmdir /s /q "$(PACKAGE_DIR)"
	@mkdir "$(PACKAGE_DIR)"
	@copy /Y "bin\sixty-four.exe" "$(PACKAGE_DIR)\sixty-four.exe" >nul
	@copy /Y "external\SFML\bin\*.dll" "$(PACKAGE_DIR)\" >nul
	@xcopy /E /I /Y "Sprites" "$(PACKAGE_DIR)\Sprites" >nul
	@xcopy /E /I /Y "Audio" "$(PACKAGE_DIR)\Audio" >nul
	@mkdir "$(PACKAGE_DIR)\data\config"
	@mkdir "$(PACKAGE_DIR)\data\saves"
	@mkdir "$(PACKAGE_DIR)\stockfish"
	@copy /Y "stockfish\stockfish-windows-x86-64-avx2.exe" "$(PACKAGE_DIR)\stockfish\stockfish-windows-x86-64-avx2.exe" >nul
	@copy /Y "stockfish\AUTHORS" "$(PACKAGE_DIR)\stockfish\AUTHORS" >nul
	@copy /Y "stockfish\Copying.txt" "$(PACKAGE_DIR)\stockfish\Copying.txt" >nul
	@copy /Y "stockfish\README.md" "$(PACKAGE_DIR)\stockfish\README.md" >nul
	@xcopy /E /I /Y "stockfish\src" "$(PACKAGE_DIR)\stockfish\src" >nul
	@copy /Y "README.md" "$(PACKAGE_DIR)\README.md" >nul
	@copy /Y "LICENSE" "$(PACKAGE_DIR)\LICENSE" >nul
	@copy /Y "THIRD_PARTY_NOTICES.md" "$(PACKAGE_DIR)\THIRD_PARTY_NOTICES.md" >nul
	@echo Package created at $(PACKAGE_DIR)

.PHONY: all clean run test package
