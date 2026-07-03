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
RELEASE_DIR = dist/Sixty-Four Release

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

release: all
	@if exist "$(RELEASE_DIR)" rmdir /s /q "$(RELEASE_DIR)"
	@mkdir "$(RELEASE_DIR)"
	@copy /Y "bin\sixty-four.exe" "$(RELEASE_DIR)\sixty-four.exe" >nul
	@copy /Y "external\SFML\bin\*.dll" "$(RELEASE_DIR)\" >nul
	@xcopy /E /I /Y "Sprites" "$(RELEASE_DIR)\Sprites" >nul
	@xcopy /E /I /Y "Audio" "$(RELEASE_DIR)\Audio" >nul
	@mkdir "$(RELEASE_DIR)\data\config"
	@mkdir "$(RELEASE_DIR)\data\saves"
	@mkdir "$(RELEASE_DIR)\stockfish"
	@copy /Y "stockfish\stockfish-windows-x86-64-avx2.exe" "$(RELEASE_DIR)\stockfish\stockfish-windows-x86-64-avx2.exe" >nul
	@copy /Y "stockfish\AUTHORS" "$(RELEASE_DIR)\stockfish\AUTHORS" >nul
	@copy /Y "stockfish\Copying.txt" "$(RELEASE_DIR)\stockfish\Copying.txt" >nul
	@copy /Y "stockfish\README.md" "$(RELEASE_DIR)\stockfish\README.md" >nul
	@xcopy /E /I /Y "stockfish\src" "$(RELEASE_DIR)\stockfish\src" >nul
	@copy /Y "RELEASE_README.txt" "$(RELEASE_DIR)\README.txt" >nul
	@copy /Y "LICENSE" "$(RELEASE_DIR)\LICENSE" >nul
	@copy /Y "THIRD_PARTY_NOTICES.md" "$(RELEASE_DIR)\THIRD_PARTY_NOTICES.md" >nul
	@echo Playable release created at $(RELEASE_DIR)

package: release
	@echo Sixty-Four playable release is ready in dist

.PHONY: all clean run test release package
