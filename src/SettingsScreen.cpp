#include "SettingsScreen.hpp"
#include "MenuPreset.hpp"
#include "UIPrimitives.hpp"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <fstream>

// ─── Palette (matches PlayerSetupScreen and game board) ────────────────────
static const sf::Color COL_BG_DARK  = sf::Color(14,  14,  18);
static const sf::Color COL_PANEL    = sf::Color(22,  22,  28);
static const sf::Color COL_BORDER   = sf::Color(70,  70,  82);
static const sf::Color COL_ACCENT   = sf::Color(173, 146, 29);
static const sf::Color COL_ACCENT_L = sf::Color(210, 185, 60);
static const sf::Color COL_TEXT     = sf::Color(220, 220, 232);
static const sf::Color COL_SUBTEXT  = sf::Color(140, 140, 158);
static const sf::Color COL_ROW_BG   = sf::Color(28,  28,  36);
static const sf::Color COL_ROW_FOC  = sf::Color(35,  35,  44);

static const std::vector<char> PIECE_TYPES = {'P', 'N', 'B', 'R', 'Q', 'K'};

struct TilePalette {
    sf::Color light;
    sf::Color dark;
};

static TilePalette tilePaletteForTheme(const std::string& theme) {
    if (theme == "Wood")          return {sf::Color(232, 195, 139), sf::Color(131, 82, 48)};
    if (theme == "Marble")        return {sf::Color(232, 232, 225), sf::Color(92, 107, 119)};
    if (theme == "Ice")           return {sf::Color(220, 240, 242), sf::Color(91, 154, 171)};
    if (theme == "Minimalist")    return {sf::Color(226, 226, 222), sf::Color(98, 102, 104)};
    if (theme == "Emerald")       return {sf::Color(235, 235, 210), sf::Color(68, 142, 94)};
    if (theme == "Slate")         return {sf::Color(205, 214, 217), sf::Color(74, 86, 98)};
    if (theme == "Royal")         return {sf::Color(232, 222, 184), sf::Color(86, 75, 145)};
    if (theme == "Rosewood")      return {sf::Color(238, 207, 195), sf::Color(145, 72, 78)};
    if (theme == "Graphite")      return {sf::Color(190, 194, 198), sf::Color(48, 52, 58)};
    if (theme == "Tournament")    return {sf::Color(238, 238, 210), sf::Color(118, 150, 86)};
    if (theme == "High Contrast") return {sf::Color(245, 245, 245), sf::Color(55, 55, 55)};
    return {sf::Color(240, 217, 181), sf::Color(181, 136, 99)};
}

static std::string pieceTypeName(char pieceType) {
    switch ((char)std::toupper((unsigned char)pieceType)) {
        case 'P': return "PAWN";
        case 'N': return "KNIGHT";
        case 'B': return "BISHOP";
        case 'R': return "ROOK";
        case 'Q': return "QUEEN";
        case 'K': return "KING";
        default:  return "PIECE";
    }
}

static std::string spriteFileName(char boardPiece) {
    char side = std::isupper((unsigned char)boardPiece) ? 'W' : 'B';
    switch ((char)std::toupper((unsigned char)boardPiece)) {
        case 'P': return std::string(1, side) + "-Pawn.png";
        case 'N': return std::string(1, side) + "-Knight.png";
        case 'B': return std::string(1, side) + "-Bishop.png";
        case 'R': return std::string(1, side) + "-Rook.png";
        case 'Q': return std::string(1, side) + "-Queen.png";
        case 'K': return std::string(1, side) + "-King.png";
        default:  return "";
    }
}

static int naturalSetNumber(const std::string& name) {
    std::string digits;
    for (char ch : name) {
        if (std::isdigit((unsigned char)ch)) digits += ch;
    }
    if (digits.empty()) return 1000000;
    try { return std::stoi(digits); } catch (...) { return 1000000; }
}

static std::string titleFromFileStem(std::string stem) {
    if (stem.empty()) return stem;

    for (char& ch : stem) {
        if (ch == '_' || ch == '-') ch = ' ';
    }

    bool newWord = true;
    for (char& ch : stem) {
        if (std::isspace((unsigned char)ch)) {
            newWord = true;
        } else if (newWord) {
            ch = (char)std::toupper((unsigned char)ch);
            newWord = false;
        } else {
            ch = (char)std::tolower((unsigned char)ch);
        }
    }
    return stem;
}

static bool isSupportedImageFile(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
        return (char)std::tolower(ch);
    });
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp";
}

static std::string lowerString(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return (char)std::tolower(ch);
    });
    return value;
}

static bool isCurFile(const std::filesystem::path& path) {
    return lowerString(path.extension().string()) == ".cur";
}

static int cursorFileScore(const std::filesystem::path& path) {
    std::string name = lowerString(path.stem().string());
    if (name.find("beyaz fil") != std::string::npos) return 0;
    if (name.find("normal select") != std::string::npos) return 1;
    if (name.find("normal") != std::string::npos) return 2;
    if (name.find("base") != std::string::npos) return 3;
    if (name.find("arrow") != std::string::npos) return 4;
    if (name.find("select") != std::string::npos) return 5;
    return 10;
}

static std::string arrowCursorFromCrs(const std::filesystem::path& crsPath) {
    std::ifstream file(crsPath);
    if (!file) return "";

    bool inArrowBlock = false;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line == "[Arrow]") {
            inArrowBlock = true;
            continue;
        }
        if (!line.empty() && line.front() == '[') {
            inArrowBlock = false;
            continue;
        }
        const std::string prefix = "Path=";
        if (inArrowBlock && line.rfind(prefix, 0) == 0 && line.size() > prefix.size()) {
            std::filesystem::path cursorPath = crsPath.parent_path() / line.substr(prefix.size());
            if (std::filesystem::exists(cursorPath) && isCurFile(cursorPath)) {
                return cursorPath.generic_string();
            }
            return "";
        }
    }

    return "";
}

static float textureFitScale(const sf::Texture& tex, float maxSize) {
    sf::Vector2u size = tex.getSize();
    float largest = (float)std::max(size.x, size.y);
    return largest > 0.f ? maxSize / largest : 1.f;
}

SettingsScreen::SettingsScreen(sf::RenderWindow& win,
                                BitmapFont& title,
                                BitmapFont& body)
    : window(win), titleFont(title), bodyFont(body)
{
    buildRows();
    scanPieceSets();
}

void SettingsScreen::buildRows() {
    rows.clear();
    scanBoardBackgrounds();
    scanCursorStyles();

    // ── Row 0 : Menu Background ────────────────────────────────────────────
    {
        SelectorRow r;
        r.label = "MENU PRESET";
        for (const auto& preset : getMenuPresets()) {
            r.options.push_back(preset.name);
        }
        if (r.options.empty()) r.options = { "Preset 1" };
        r.currentIndex = std::min(1, (int)r.options.size() - 1);
        rows.push_back(r);
    }
    // ── Row 1 : Board Tile Theme ───────────────────────────────────────────
    {
        SelectorRow r;
        r.label = "BOARD TILE THEME";
        r.options = { "Classic", "Wood", "Marble", "Ice", "Minimalist",
                      "Emerald", "Slate", "Royal", "Rosewood",
                      "Graphite", "Tournament", "High Contrast" };
        r.currentIndex = 0;
        rows.push_back(r);
    }
    // ── Row 2 : Board Background ───────────────────────────────────────────
    {
        SelectorRow r;
        r.label = "BOARD BACKGROUND";
        for (const auto& [label, path] : boardBackgroundFiles) {
            (void)path;
            r.options.push_back(label);
        }
        std::sort(r.options.begin(), r.options.end(), [](const std::string& a, const std::string& b) {
            if (a == "Wood") return true;
            if (b == "Wood") return false;
            return a < b;
        });
        r.options.push_back("Plain");
        if (r.options.empty()) r.options = { "Plain" };
        r.currentIndex = 0;
        rows.push_back(r);
    }
    // ── Row 3 : Board Perspective ──────────────────────────────────────────
    {
        SelectorRow r;
        r.label = "BOARD PERSPECTIVE";
        r.options = { "White", "Black", "Auto" };
        r.currentIndex = 0;
        rows.push_back(r);
    }
    {
        SelectorRow r;
        r.label = "CURSOR STYLE";
        for (const auto& [label, path] : cursorFiles) {
            (void)path;
            r.options.push_back(label);
        }
        std::sort(r.options.begin(), r.options.end(), [](const std::string& a, const std::string& b) {
            if (a == "Chess") return true;
            if (b == "Chess") return false;
            int an = naturalSetNumber(a);
            int bn = naturalSetNumber(b);
            if (an != bn) return an < bn;
            return a < b;
        });
        r.options.push_back("System");
        r.currentIndex = 0;
        rows.push_back(r);
    }
    {
        SelectorRow r;
        r.label = "FAHH MODE";
        r.options = { "Off", "On" };
        r.currentIndex = 0;
        rows.push_back(r);
    }
    // ── Row 4 : Base Time ──────────────────────────────────────────────────
    {
        SelectorRow r;
        r.label = "BASE TIME";
        r.options = { "1 Min", "3 Min", "5 Min", "10 Min", "15 Min", "30 Min", "60 Min" };
        r.currentIndex = 3; // Default to 10 Min
        rows.push_back(r);
    }
    // ── Row 5 : Time Increment ─────────────────────────────────────────────
    {
        SelectorRow r;
        r.label = "INCREMENT (SEC)";
        r.options = { "0 Sec", "1 Sec", "2 Sec", "3 Sec", "5 Sec", "10 Sec", "15 Sec", "30 Sec" };
        r.currentIndex = 0;
        rows.push_back(r);
    }
    // ── Row 6 : Time Delay ──────────────────────────────────────────────────
    {
        SelectorRow r;
        r.label = "DELAY (SEC)";
        r.options = { "0 Sec", "1 Sec", "2 Sec", "3 Sec", "5 Sec", "10 Sec", "15 Sec", "30 Sec" };
        r.currentIndex = 0;
        rows.push_back(r);
    }

    rows.resize(6);
}

void SettingsScreen::scanBoardBackgrounds() {
    boardBackgroundFiles.clear();
    boardBackgroundTextures.clear();

    const std::filesystem::path root("Sprites/Game bg");
    std::vector<std::pair<std::string, std::string>> backgrounds;
    if (std::filesystem::exists(root) && std::filesystem::is_directory(root)) {
        for (const auto& entry : std::filesystem::directory_iterator(root)) {
            if (!entry.is_regular_file() || !isSupportedImageFile(entry.path())) continue;
            std::string label = titleFromFileStem(entry.path().stem().string());
            backgrounds.push_back({label, entry.path().generic_string()});
        }
    }

    std::sort(backgrounds.begin(), backgrounds.end(), [](const auto& a, const auto& b) {
        if (a.first == "Wood") return true;
        if (b.first == "Wood") return false;
        return a.first < b.first;
    });

    for (const auto& item : backgrounds) {
        boardBackgroundFiles[item.first] = item.second;
        sf::Texture texture;
        if (texture.loadFromFile(item.second)) {
            boardBackgroundTextures[item.first] = texture;
        }
    }
}

void SettingsScreen::scanCursorStyles() {
    cursorFiles.clear();

    const std::filesystem::path root("Sprites/Cursor");
    std::vector<std::pair<std::string, std::string>> styles;
    if (std::filesystem::exists(root) && std::filesystem::is_directory(root)) {
        for (const auto& entry : std::filesystem::directory_iterator(root)) {
            std::vector<std::filesystem::path> curFiles;
            std::string label;
            std::string crsArrowPath;

            if (entry.is_directory()) {
                label = titleFromFileStem(entry.path().filename().string());
                for (const auto& file : std::filesystem::directory_iterator(entry.path())) {
                    if (file.is_regular_file() && isCurFile(file.path())) {
                        curFiles.push_back(file.path());
                    } else if (file.is_regular_file() && lowerString(file.path().extension().string()) == ".crs") {
                        std::string arrowPath = arrowCursorFromCrs(file.path());
                        if (!arrowPath.empty()) crsArrowPath = arrowPath;
                    }
                }
            } else if (entry.is_regular_file() && isCurFile(entry.path())) {
                label = titleFromFileStem(entry.path().stem().string());
                curFiles.push_back(entry.path());
            }

            if (curFiles.empty()) continue;

            std::sort(curFiles.begin(), curFiles.end(), [](const auto& a, const auto& b) {
                int as = cursorFileScore(a);
                int bs = cursorFileScore(b);
                if (as != bs) return as < bs;
                int an = naturalSetNumber(a.stem().string());
                int bn = naturalSetNumber(b.stem().string());
                if (an != bn) return an < bn;
                return a.filename().string() < b.filename().string();
            });

            if (!label.empty()) {
                styles.push_back({label, crsArrowPath.empty() ? curFiles.front().generic_string() : crsArrowPath});
            }
        }
    }

    std::sort(styles.begin(), styles.end(), [](const auto& a, const auto& b) {
        if (a.first == "Chess") return true;
        if (b.first == "Chess") return false;
        int an = naturalSetNumber(a.first);
        int bn = naturalSetNumber(b.first);
        if (an != bn) return an < bn;
        return a.first < b.first;
    });

    for (const auto& style : styles) {
        cursorFiles[style.first] = style.second;
    }
}

void SettingsScreen::scanPieceSets() {
    std::string oldAppliedName;
    if (!pieceSets.empty() && appliedSetIndex >= 0 && appliedSetIndex < (int)pieceSets.size()) {
        oldAppliedName = pieceSets[appliedSetIndex].displayName;
    }

    std::map<char, std::string> oldCustomNames;
    for (char piece : PIECE_TYPES) {
        auto it = appliedCustomSetIndex.find(piece);
        if (it != appliedCustomSetIndex.end() && it->second >= 0 && it->second < (int)pieceSets.size()) {
            oldCustomNames[piece] = pieceSets[it->second].displayName;
        }
    }

    pieceSets.clear();

    const std::filesystem::path root("Sprites/Pieces");
    if (std::filesystem::exists(root) && std::filesystem::is_directory(root)) {
        for (const auto& entry : std::filesystem::directory_iterator(root)) {
            if (!entry.is_directory()) continue;

            PieceSetInfo set;
            set.displayName = entry.path().filename().string();
            set.directory = entry.path().generic_string();

            bool hasAnyPiece = false;
            const char pieces[] = {'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'};
            for (char piece : pieces) {
                std::string fileName = spriteFileName(piece);
                std::filesystem::path filePath = entry.path() / fileName;
                if (std::filesystem::exists(filePath)) {
                    hasAnyPiece = true;
                    set.files[piece] = filePath.generic_string();
                    sf::Texture tex;
                    if (tex.loadFromFile(set.files[piece])) {
                        set.textures[piece] = tex;
                    }
                }
            }

            if (hasAnyPiece) {
                pieceSets.push_back(set);
            }
        }
    }

    std::sort(pieceSets.begin(), pieceSets.end(), [](const PieceSetInfo& a, const PieceSetInfo& b) {
        int an = naturalSetNumber(a.displayName);
        int bn = naturalSetNumber(b.displayName);
        if (an != bn) return an < bn;
        return a.displayName < b.displayName;
    });

    if (!oldAppliedName.empty()) {
        int idx = findPieceSetIndex(oldAppliedName);
        if (idx >= 0) appliedSetIndex = idx;
    }
    appliedSetIndex = std::clamp(appliedSetIndex, 0, std::max(0, (int)pieceSets.size() - 1));
    draftSetIndex = appliedSetIndex;

    for (char piece : PIECE_TYPES) {
        int customIdx = appliedCustomSetIndex.count(piece) ? appliedCustomSetIndex[piece] : appliedSetIndex;
        auto nameIt = oldCustomNames.find(piece);
        if (nameIt != oldCustomNames.end()) {
            int idx = findPieceSetIndex(nameIt->second);
            if (idx >= 0) customIdx = idx;
        }
        customIdx = std::clamp(customIdx, 0, std::max(0, (int)pieceSets.size() - 1));
        appliedCustomSetIndex[piece] = customIdx;
        draftCustomSetIndex[piece] = customIdx;
    }
}

// ─── Events ───────────────────────────────────────────────────────────────
void SettingsScreen::handleEvent(const sf::Event& event) {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;

    if (pieceModalOpen) {
        handlePieceModalEvent(event);
        return;
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape)
            pendingAction = SettingsAction::Back;
        if (event.key.code == sf::Keyboard::Escape)
            clickSoundRequested = true;
        if (event.key.code == sf::Keyboard::Up) {
            focusedRow = std::max(0, focusedRow - 1);
            changeSoundRequested = true;
        }
        if (event.key.code == sf::Keyboard::Down) {
            focusedRow = std::min(std::min(5, (int)rows.size() - 1), focusedRow + 1);
            changeSoundRequested = true;
        }
        if (event.key.code == sf::Keyboard::Left) {
            rows[focusedRow].prev();
            rows[focusedRow].leftScale = 1.35f;   // trigger bulge
            rows[focusedRow].valuePulse = 1.0f;
            changeSoundRequested = true;
        }
        if (event.key.code == sf::Keyboard::Right) {
            rows[focusedRow].next();
            rows[focusedRow].rightScale = 1.35f;
            rows[focusedRow].valuePulse = 1.0f;
            changeSoundRequested = true;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) {

        sf::Vector2f mp(event.mouseButton.x, event.mouseButton.y);
        float rowH    = 54.f;
        float rowGap  = 10.f;
        float panelW  = W * 0.52f;
        float panelX  = W / 2.f - panelW / 2.f;
        float startY  = H * 0.22f;

        int visibleRows = std::min(6, (int)rows.size());
        for (int i = 0; i < visibleRows; ++i) {
            float rowY = startY + i * (rowH + rowGap);
            float rowCX = panelX + panelW / 2.f;

            // Left arrow hit area
            sf::FloatRect leftRect(rowCX - 180.f, rowY + 7.f, 40.f, 40.f);
            // Right arrow hit area
            sf::FloatRect rightRect(rowCX + 140.f, rowY + 7.f, 40.f, 40.f);

            if (leftRect.contains(mp)) {
                focusedRow = i;
                rows[i].prev();
                rows[i].leftScale = 1.35f;
                rows[i].valuePulse = 1.0f;
                changeSoundRequested = true;
            } else if (rightRect.contains(mp)) {
                focusedRow = i;
                rows[i].next();
                rows[i].rightScale = 1.35f;
                rows[i].valuePulse = 1.0f;
                changeSoundRequested = true;
            } else if (sf::FloatRect(panelX, rowY, panelW, rowH).contains(mp)) {
                if (focusedRow != i) {
                    focusedRow = i;
                    changeSoundRequested = true;
                }
            }
        }

        // Back button
        if (sf::FloatRect(40.f, 30.f, 100.f, 36.f).contains(mp)) {
            pendingAction = SettingsAction::Back;
            clickSoundRequested = true;
        }

        if (getPieceButtonRect().contains(mp)) {
            clickSoundRequested = true;
            openPieceModal();
        }
    }
}

bool SettingsScreen::consumeChangeSound() {
    bool requested = changeSoundRequested;
    changeSoundRequested = false;
    return requested;
}

bool SettingsScreen::consumeClickSound() {
    bool requested = clickSoundRequested;
    clickSoundRequested = false;
    return requested;
}

// ─── Update ───────────────────────────────────────────────────────────────
void SettingsScreen::update(float dt) {
    animTimer += dt;

    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mp(mi);
    backHover += (sf::FloatRect(40.f, 30.f, 100.f, 36.f).contains(mp) ? dt : -dt) * 4.f;
    backHover  = std::clamp(backHover, 0.f, 1.f);
    pieceButtonHover += (getPieceButtonRect().contains(mp) && !pieceModalOpen ? dt : -dt) * 5.f;
    pieceButtonHover  = std::clamp(pieceButtonHover, 0.f, 1.f);

    // Decay bulge scale back to 1.0 smoothly
    for (int i = 0; i < (int)rows.size(); ++i) {
        auto& row = rows[i];
        row.leftScale  += (1.0f - row.leftScale)  * dt * 12.f;
        row.rightScale += (1.0f - row.rightScale) * dt * 12.f;
        row.focusAmount = ui::smoothToward(row.focusAmount, i == focusedRow ? 1.f : 0.f, dt, 10.f);
        row.valuePulse = ui::smoothToward(row.valuePulse, 0.f, dt, 7.f);
    }
}

// ─── Draw helpers ─────────────────────────────────────────────────────────
void SettingsScreen::drawBackground() {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;

    sf::RectangleShape bg({W, H});
    bg.setFillColor(COL_BG_DARK);
    window.draw(bg);

    // Decorative large dim circle accent
    sf::CircleShape glow(H * 0.55f);
    glow.setOrigin(H * 0.55f, H * 0.55f);
    glow.setPosition(W / 2.f, H / 2.f);
    glow.setFillColor(sf::Color(173, 146, 29, 5));
    window.draw(glow);
}

void SettingsScreen::drawArrow(float cx, float cy, bool pointRight,
                                float scale, bool hovered) {
    // Arrow drawn as a simple triangle using sf::ConvexShape for clean rendering
    sf::ConvexShape arrow(3);
    float s = 14.f * scale;
    if (pointRight) {
        arrow.setPoint(0, {cx - s * 0.5f, cy - s});
        arrow.setPoint(1, {cx - s * 0.5f, cy + s});
        arrow.setPoint(2, {cx + s * 0.5f, cy});
    } else {
        arrow.setPoint(0, {cx + s * 0.5f, cy - s});
        arrow.setPoint(1, {cx + s * 0.5f, cy + s});
        arrow.setPoint(2, {cx - s * 0.5f, cy});
    }
    arrow.setFillColor(hovered ? COL_ACCENT_L : (scale > 1.05f ? COL_ACCENT : COL_SUBTEXT));
    window.draw(arrow);
}

void SettingsScreen::drawRow(SelectorRow& row, float cx, float y, bool focused) {
    (void)focused;
    float W      = (float)window.getSize().x;
    float panelW = W * 0.52f;
    float rowH   = 54.f;
    float rowX   = cx - panelW / 2.f;

    ui::drawRoundedRect(window, {rowX + 4, y + 5, panelW, rowH}, 6.f,
                        sf::Color(0, 0, 0, 50));
    float f = row.focusAmount;
    sf::Color rowFill = ui::mixColor(COL_ROW_BG, COL_ROW_FOC, f);
    sf::Color rowOutline = ui::mixColor(COL_BORDER, COL_ACCENT, f);
    ui::drawRoundedPanel(window, {rowX, y, panelW, rowH}, 6.f,
                         rowFill, rowOutline, 1.0f + 0.5f * f);

    // Left accent bar when focused
    if (f > 0.02f) {
        sf::RectangleShape bar({3.f, rowH});
        bar.setPosition(rowX, y);
        bar.setFillColor(sf::Color(COL_ACCENT.r, COL_ACCENT.g, COL_ACCENT.b, (sf::Uint8)(220 * f)));
        window.draw(bar);
    }

    sf::Vector2i mi = sf::Mouse::getPosition(window);
    sf::Vector2f mp(mi);
    float arrowCY = y + rowH / 2.f;

    // ← Left arrow
    float leftX = cx - 155.f;
    bool lHov = sf::FloatRect(leftX - 20, arrowCY - 20, 40, 40).contains(mp);
    drawArrow(leftX, arrowCY, false, row.leftScale, lHov);

    // → Right arrow
    float rightX = cx + 155.f;
    bool rHov = sf::FloatRect(rightX - 20, arrowCY - 20, 40, 40).contains(mp);
    drawArrow(rightX, arrowCY, true, row.rightScale, rHov);

    if (row.label == "MENU PRESET" || row.label == "BOARD TILE THEME" || row.label == "BOARD BACKGROUND") {
        float pvW = 58.f, pvH = 34.f;
        float pvX = rowX + panelW - pvW - 18.f;
        float pvY = y + (rowH - pvH) / 2.f;
        ui::drawRoundedPanel(window, {pvX, pvY, pvW, pvH}, 4.f,
                             sf::Color(16, 16, 22, 235),
                             ui::mixColor(sf::Color(80, 80, 92), COL_ACCENT, f),
                             1.0f);

        sf::FloatRect inner(pvX + 6.f, pvY + 6.f, pvW - 12.f, pvH - 12.f);
        if (row.label == "BOARD TILE THEME") {
            TilePalette palette = tilePaletteForTheme(row.current());
            float tile = inner.height / 2.f;
            for (int rr = 0; rr < 2; ++rr) {
                for (int cc = 0; cc < 3; ++cc) {
                    sf::RectangleShape sq({tile, tile});
                    sq.setPosition(inner.left + cc * tile, inner.top + rr * tile);
                    sf::Color fill = ((rr + cc) % 2 == 0) ? palette.light : palette.dark;
                    fill.a = 220;
                    sq.setFillColor(fill);
                    window.draw(sq);
                }
            }
        } else if (row.label == "BOARD BACKGROUND") {
            auto texIt = boardBackgroundTextures.find(row.current());
            if (texIt != boardBackgroundTextures.end()) {
                const sf::Texture& texture = texIt->second;
                sf::Sprite preview(texture);
                sf::Vector2u size = texture.getSize();
                float scale = std::min(inner.width / (float)size.x, inner.height / (float)size.y);
                preview.setScale(scale, scale);
                sf::FloatRect bounds = preview.getGlobalBounds();
                preview.setPosition(inner.left + inner.width / 2.f - bounds.width / 2.f,
                                    inner.top + inner.height / 2.f - bounds.height / 2.f);
                window.draw(preview);
            } else {
                ui::drawRoundedRect(window, inner, 3.f, sf::Color(25, 25, 30, 230));
                sf::RectangleShape slash({inner.width, 2.f});
                slash.setOrigin(inner.width / 2.f, 1.f);
                slash.setPosition(inner.left + inner.width / 2.f, inner.top + inner.height / 2.f);
                slash.setRotation(-22.f);
                slash.setFillColor(sf::Color(120, 120, 132, 160));
                window.draw(slash);
            }
        } else {
            ui::drawRoundedRect(window, inner, 3.f,
                                sf::Color(38, 35, 45, 210));
            sf::RectangleShape stripe({inner.width, 2.f});
            stripe.setPosition(inner.left, inner.top + inner.height * 0.6f);
            stripe.setFillColor(sf::Color(210, 185, 80, 80));
            window.draw(stripe);
        }
    }

    // Row label (smaller, subtle)
    float lScale = 0.14f;
    bodyFont.drawText(window, row.label,
                      {rowX + 16.f, y + 5.f},
                      lScale, COL_SUBTEXT);

    // Current value (prominent, centered)
    float vScale = 0.22f;
    float vw = bodyFont.getTextWidth(row.current(), vScale);
    while (vw > panelW * 0.42f && vScale > 0.14f) {
        vScale -= 0.01f;
        vw = bodyFont.getTextWidth(row.current(), vScale);
    }
    float pulseScale = vScale + row.valuePulse * 0.018f;
    vw = bodyFont.getTextWidth(row.current(), pulseScale);
    bodyFont.drawText(window, row.current(),
                      {cx - vw / 2.f, y + 17.f},
                      pulseScale, ui::mixColor(sf::Color(190, 190, 200), COL_TEXT, f));
}

void SettingsScreen::drawButton(const std::string& label,
                                 sf::FloatRect rect, float hoverAmount) {
    ui::drawRoundedPanel(window, rect, 5.f,
                         ui::mixColor(sf::Color(28, 28, 36), COL_ACCENT, hoverAmount),
                         ui::mixColor(COL_BORDER, COL_ACCENT_L, hoverAmount),
                         1.5f);

    float scale = 0.20f;
    float tw = bodyFont.getTextWidth(label, scale);
    bodyFont.drawText(window, label,
                      {rect.left + rect.width / 2.f - tw / 2.f,
                       rect.top + (rect.height - scale * 50.f) / 2.f},
                      scale, ui::mixColor(COL_TEXT, sf::Color(10, 10, 12), hoverAmount));
}

// ─── Main draw ────────────────────────────────────────────────────────────
void SettingsScreen::drawLargeSelectionPreview() {
    if (rows.empty() || focusedRow < 0 || focusedRow >= (int)rows.size()) return;

    const SelectorRow& row = rows[focusedRow];
    if (row.label != "BOARD TILE THEME" && row.label != "BOARD BACKGROUND") return;

    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;
    float selectorW = W * 0.52f;
    float selectorRight = W / 2.f + selectorW / 2.f;
    float panelX = selectorRight + 28.f;
    float panelW = W - panelX - 40.f;
    if (panelW < 190.f) return;

    panelW = std::min(panelW, 360.f);
    float panelH = std::min(360.f, H * 0.48f);
    float panelY = H * 0.22f;

    ui::drawRoundedRect(window, {panelX + 6.f, panelY + 8.f, panelW, panelH}, 8.f,
                        sf::Color(0, 0, 0, 70));
    ui::drawRoundedPanel(window, {panelX, panelY, panelW, panelH}, 8.f,
                         sf::Color(20, 20, 26, 242),
                         sf::Color(90, 82, 58, 210),
                         1.2f);

    bodyFont.drawText(window, row.label, {panelX + 16.f, panelY + 14.f}, 0.12f, COL_SUBTEXT);

    float valueScale = 0.18f;
    while (bodyFont.getTextWidth(row.current(), valueScale) > panelW - 32.f && valueScale > 0.12f) {
        valueScale -= 0.01f;
    }
    bodyFont.drawText(window, row.current(), {panelX + 16.f, panelY + 39.f}, valueScale, COL_TEXT);

    sf::FloatRect previewRect(panelX + 16.f, panelY + 76.f, panelW - 32.f, panelH - 94.f);
    ui::drawRoundedPanel(window, previewRect, 6.f,
                         sf::Color(12, 12, 16, 245),
                         sf::Color(58, 58, 70, 230),
                         1.f);

    sf::FloatRect inner(previewRect.left + 10.f, previewRect.top + 10.f,
                        previewRect.width - 20.f, previewRect.height - 20.f);

    if (row.label == "BOARD TILE THEME") {
        TilePalette palette = tilePaletteForTheme(row.current());
        int cells = 6;
        float boardSize = std::min(inner.width, inner.height);
        float tile = boardSize / (float)cells;
        float startX = inner.left + inner.width / 2.f - boardSize / 2.f;
        float startY = inner.top + inner.height / 2.f - boardSize / 2.f;

        for (int rr = 0; rr < cells; ++rr) {
            for (int cc = 0; cc < cells; ++cc) {
                sf::RectangleShape sq({tile + 0.5f, tile + 0.5f});
                sq.setPosition(startX + cc * tile, startY + rr * tile);
                sq.setFillColor(((rr + cc) % 2 == 0) ? palette.light : palette.dark);
                window.draw(sq);
            }
        }

        sf::RectangleShape focus({tile, tile});
        focus.setPosition(startX + tile * 2.f, startY + tile * 2.f);
        focus.setFillColor(sf::Color(COL_ACCENT.r, COL_ACCENT.g, COL_ACCENT.b, 165));
        window.draw(focus);

        sf::RectangleShape outline({boardSize, boardSize});
        outline.setPosition(startX, startY);
        outline.setFillColor(sf::Color::Transparent);
        outline.setOutlineThickness(1.5f);
        outline.setOutlineColor(sf::Color(230, 210, 125, 135));
        window.draw(outline);
    } else {
        auto texIt = boardBackgroundTextures.find(row.current());
        if (texIt != boardBackgroundTextures.end()) {
            const sf::Texture& texture = texIt->second;
            sf::Vector2u size = texture.getSize();
            if (size.x > 0 && size.y > 0) {
                sf::Sprite preview(texture);
                float scale = std::min(inner.width / (float)size.x, inner.height / (float)size.y);
                preview.setScale(scale, scale);
                sf::FloatRect bounds = preview.getGlobalBounds();
                preview.setPosition(inner.left + inner.width / 2.f - bounds.width / 2.f,
                                    inner.top + inner.height / 2.f - bounds.height / 2.f);
                window.draw(preview);
            }
        } else {
            ui::drawRoundedRect(window, inner, 5.f, sf::Color(26, 26, 32, 245));
            sf::RectangleShape slash({inner.width * 0.72f, 3.f});
            slash.setOrigin(inner.width * 0.36f, 1.5f);
            slash.setPosition(inner.left + inner.width / 2.f, inner.top + inner.height / 2.f);
            slash.setRotation(-24.f);
            slash.setFillColor(sf::Color(120, 120, 132, 170));
            window.draw(slash);
        }
    }
}

void SettingsScreen::draw() {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;

    drawBackground();

    // ── Title ─────────────────────────────────────────────────────────────
    std::string title = "PERSONALIZATION";
    float tScale = 0.40f;
    float tw = bodyFont.getTextWidth(title, tScale);
    bodyFont.drawText(window, title,
                       {W / 2.f - tw / 2.f, H * 0.05f},
                       tScale, sf::Color(210, 185, 60));

    std::string sub = "SETTINGS";
    float subScale = 0.22f;
    float sw = bodyFont.getTextWidth(sub, subScale);
    bodyFont.drawText(window, sub,
                      {W / 2.f - sw / 2.f, H * 0.10f},
                      subScale, COL_SUBTEXT);

    // Gold divider
    sf::RectangleShape div({220.f, 1.5f});
    div.setPosition(W / 2.f - 110.f, H * 0.14f);
    div.setFillColor(COL_ACCENT);
    window.draw(div);

    // ── Section header: PERSONALIZATION ───────────────────────────────────
    std::string sec = "GAME OPTIONS & STYLES";
    float secScale = 0.16f;
    float secW = bodyFont.getTextWidth(sec, secScale);
    bodyFont.drawText(window, sec,
                      {W / 2.f - secW / 2.f, H * 0.17f},
                      secScale, COL_SUBTEXT);

    // ── Selector rows ─────────────────────────────────────────────────────
    float rowH   = 54.f;
    float rowGap = 10.f;
    float startY = H * 0.22f;
    float cx     = W / 2.f;

    int visibleRows = std::min(6, (int)rows.size());
    for (int i = 0; i < visibleRows; ++i) {
        drawRow(rows[i], cx, startY + i * (rowH + rowGap), i == focusedRow);
    }

    drawLargeSelectionPreview();

    drawButton("CHANGE CHESS PIECES", getPieceButtonRect(), pieceButtonHover);

    // ── Hint ──────────────────────────────────────────────────────────────
    std::string hint = "UP/DOWN to navigate  |  LEFT/RIGHT to change  |  CLICK arrows";
    float hScale = 0.13f;
    float hw = bodyFont.getTextWidth(hint, hScale);
    bodyFont.drawText(window, hint,
                      {W / 2.f - hw / 2.f, H * 0.88f},
                      hScale, COL_SUBTEXT);

    // ── Back button ───────────────────────────────────────────────────────
    drawButton("< BACK", {40.f, 30.f, 100.f, 36.f}, backHover);
    drawPieceModal();
    // Note: window.display() called by Game::render()
}

void SettingsScreen::setSelection(int rowIdx, const std::string& val) {
    if (rowIdx < 0 || rowIdx >= (int)rows.size()) return;
    auto& r = rows[rowIdx];
    for (int i = 0; i < (int)r.options.size(); ++i) {
        if (r.options[i] == val) {
            r.currentIndex = i;
            break;
        }
    }

    if (rowIdx == 2 && (val == "Texture" || val == "Wood Table")) {
        setSelection(rowIdx, "Wood");
    }

    if (rowIdx == 4 && val == "Chess" && getCursorPath().empty() && !r.options.empty()) {
        for (int i = 0; i < (int)r.options.size(); ++i) {
            if (r.options[i] != "System") {
                r.currentIndex = i;
                break;
            }
        }
    }
}

std::string SettingsScreen::getBoardBgPath() const {
    const std::string& selected = getBoardBg();
    auto it = boardBackgroundFiles.find(selected);
    if (it != boardBackgroundFiles.end()) return it->second;
    return "";
}

std::string SettingsScreen::getCursorPath() const {
    const std::string& selected = getCursorStyle();
    auto it = cursorFiles.find(selected);
    if (it != cursorFiles.end()) return it->second;
    return "";
}

sf::FloatRect SettingsScreen::getPieceButtonRect() const {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;
    float panelW = W * 0.52f;
    float rowH = 54.f;
    float rowGap = 10.f;
    float startY = H * 0.22f;
    return {W / 2.f - panelW / 2.f, startY + 6.f * (rowH + rowGap) + 12.f, panelW, 46.f};
}

void SettingsScreen::openPieceModal() {
    scanPieceSets();
    draftCustomizePieces = appliedCustomizePieces;
    draftSetIndex = appliedSetIndex;
    draftCustomSetIndex = appliedCustomSetIndex;
    for (char piece : PIECE_TYPES) {
        if (!draftCustomSetIndex.count(piece)) draftCustomSetIndex[piece] = draftSetIndex;
    }
    pieceModalOpen = true;
}

void SettingsScreen::applyPieceDraft() {
    appliedCustomizePieces = draftCustomizePieces;
    appliedSetIndex = std::clamp(draftSetIndex, 0, std::max(0, (int)pieceSets.size() - 1));
    appliedCustomSetIndex = draftCustomSetIndex;
    for (char piece : PIECE_TYPES) {
        appliedCustomSetIndex[piece] = std::clamp(appliedCustomSetIndex[piece], 0,
                                                  std::max(0, (int)pieceSets.size() - 1));
    }
    pieceSpritesChanged = true;
    pieceModalOpen = false;
}

void SettingsScreen::cycleDraftSet(int delta) {
    int optionCount = (int)pieceSets.size() + 1;
    if (optionCount <= 0) return;
    int current = draftCustomizePieces ? (int)pieceSets.size() : draftSetIndex;
    current = (current + delta + optionCount) % optionCount;
    draftCustomizePieces = current == (int)pieceSets.size();
    if (!draftCustomizePieces) draftSetIndex = current;
}

void SettingsScreen::cycleDraftCustom(char pieceType, int delta) {
    if (pieceSets.empty()) return;
    char key = (char)std::toupper((unsigned char)pieceType);
    int current = draftCustomSetIndex.count(key) ? draftCustomSetIndex[key] : draftSetIndex;
    current = (current + delta + (int)pieceSets.size()) % (int)pieceSets.size();
    draftCustomSetIndex[key] = current;
}

int SettingsScreen::findPieceSetIndex(const std::string& name) const {
    for (int i = 0; i < (int)pieceSets.size(); ++i) {
        if (pieceSets[i].displayName == name) return i;
    }
    return -1;
}

int SettingsScreen::getAppliedSetIndexForPiece(char pieceType) const {
    char key = (char)std::toupper((unsigned char)pieceType);
    auto it = appliedCustomSetIndex.find(key);
    int idx = it != appliedCustomSetIndex.end() ? it->second : appliedSetIndex;
    return std::clamp(idx, 0, std::max(0, (int)pieceSets.size() - 1));
}

std::string SettingsScreen::getPieceMode() const {
    return appliedCustomizePieces ? "Custom" : "Set";
}

std::string SettingsScreen::getPieceSetName() const {
    if (pieceSets.empty()) return "";
    int idx = std::clamp(appliedSetIndex, 0, (int)pieceSets.size() - 1);
    return pieceSets[idx].displayName;
}

std::string SettingsScreen::getCustomPieceSetName(char pieceType) const {
    if (pieceSets.empty()) return "";
    int idx = getAppliedSetIndexForPiece(pieceType);
    return pieceSets[idx].displayName;
}

std::string SettingsScreen::getPieceTexturePath(char boardPiece) const {
    if (pieceSets.empty()) return "";

    char type = (char)std::toupper((unsigned char)boardPiece);
    int setIdx = appliedCustomizePieces ? getAppliedSetIndexForPiece(type) : appliedSetIndex;
    setIdx = std::clamp(setIdx, 0, (int)pieceSets.size() - 1);

    auto it = pieceSets[setIdx].files.find(boardPiece);
    if (it != pieceSets[setIdx].files.end()) return it->second;
    return "";
}

void SettingsScreen::setPieceMode(const std::string& mode) {
    appliedCustomizePieces = mode == "Custom" || mode == "Customize";
    draftCustomizePieces = appliedCustomizePieces;
}

void SettingsScreen::setPieceSetName(const std::string& name) {
    int idx = findPieceSetIndex(name);
    if (idx >= 0) {
        appliedSetIndex = idx;
        draftSetIndex = idx;
    }
}

void SettingsScreen::setCustomPieceSetName(char pieceType, const std::string& name) {
    int idx = findPieceSetIndex(name);
    if (idx >= 0) {
        char key = (char)std::toupper((unsigned char)pieceType);
        appliedCustomSetIndex[key] = idx;
        draftCustomSetIndex[key] = idx;
    }
}

bool SettingsScreen::consumePieceSpritesChanged() {
    bool changed = pieceSpritesChanged;
    pieceSpritesChanged = false;
    return changed;
}

void SettingsScreen::handlePieceModalEvent(const sf::Event& event) {
    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;
    float cardW = std::min(820.f, W * 0.76f);
    float cardH = std::min(640.f, H * 0.82f);
    float cardX = W / 2.f - cardW / 2.f;
    float cardY = H / 2.f - cardH / 2.f;

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) {
            pieceModalOpen = false;
            clickSoundRequested = true;
        }
        if (event.key.code == sf::Keyboard::Left) {
            cycleDraftSet(-1);
            changeSoundRequested = true;
        }
        if (event.key.code == sf::Keyboard::Right) {
            cycleDraftSet(1);
            changeSoundRequested = true;
        }
        if (event.key.code == sf::Keyboard::Return) {
            applyPieceDraft();
            clickSoundRequested = true;
        }
        return;
    }

    if (event.type != sf::Event::MouseButtonPressed ||
        event.mouseButton.button != sf::Mouse::Left) {
        return;
    }

    sf::Vector2f mp = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});

    sf::FloatRect closeRect(cardX + cardW - 48.f, cardY + 22.f, 28.f, 28.f);
    sf::FloatRect selectorLeft(cardX + 215.f, cardY + 92.f, 38.f, 38.f);
    sf::FloatRect selectorRight(cardX + cardW - 78.f, cardY + 92.f, 38.f, 38.f);
    sf::FloatRect applyRect(cardX + cardW - 132.f, cardY + cardH - 54.f, 104.f, 36.f);
    sf::FloatRect cancelRect(cardX + cardW - 248.f, cardY + cardH - 54.f, 104.f, 36.f);

    if (closeRect.contains(mp) || cancelRect.contains(mp)) {
        pieceModalOpen = false;
        clickSoundRequested = true;
        return;
    }
    if (applyRect.contains(mp)) {
        applyPieceDraft();
        clickSoundRequested = true;
        return;
    }
    if (selectorLeft.contains(mp)) {
        cycleDraftSet(-1);
        changeSoundRequested = true;
        return;
    }
    if (selectorRight.contains(mp)) {
        cycleDraftSet(1);
        changeSoundRequested = true;
        return;
    }

    if (draftCustomizePieces) {
        float listY = cardY + 164.f;
        float rowH = 58.f;
        float rowGap = 8.f;
        for (int i = 0; i < (int)PIECE_TYPES.size(); ++i) {
            float rowY = listY + i * (rowH + rowGap);
            sf::FloatRect leftRect(cardX + cardW - 255.f, rowY + 11.f, 34.f, 34.f);
            sf::FloatRect rightRect(cardX + cardW - 55.f, rowY + 11.f, 34.f, 34.f);
            if (leftRect.contains(mp)) {
                cycleDraftCustom(PIECE_TYPES[i], -1);
                changeSoundRequested = true;
                return;
            }
            if (rightRect.contains(mp)) {
                cycleDraftCustom(PIECE_TYPES[i], 1);
                changeSoundRequested = true;
                return;
            }
        }
    }
}

void SettingsScreen::drawPieceModal() {
    if (!pieceModalOpen) return;

    float W = (float)window.getSize().x;
    float H = (float)window.getSize().y;
    float cardW = std::min(820.f, W * 0.76f);
    float cardH = std::min(640.f, H * 0.82f);
    float cardX = W / 2.f - cardW / 2.f;
    float cardY = H / 2.f - cardH / 2.f;

    sf::RectangleShape veil({W, H});
    veil.setFillColor(sf::Color(5, 5, 8, 185));
    window.draw(veil);

    ui::drawRoundedRect(window, {cardX + 8.f, cardY + 10.f, cardW, cardH}, 8.f, sf::Color(0, 0, 0, 90));
    ui::drawRoundedPanel(window, {cardX, cardY, cardW, cardH}, 8.f,
                         sf::Color(20, 20, 26, 248), sf::Color(120, 105, 52), 1.5f);

    bodyFont.drawText(window, "CHESS PIECES", {cardX + 28.f, cardY + 24.f}, 0.26f, COL_ACCENT_L);
    bodyFont.drawText(window, "CHANGE SET", {cardX + 30.f, cardY + 98.f}, 0.14f, COL_SUBTEXT);

    sf::Vector2f mp = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::FloatRect closeRect(cardX + cardW - 48.f, cardY + 22.f, 28.f, 28.f);
    bool closeHover = closeRect.contains(mp);
    ui::drawRoundedPanel(window, closeRect, 5.f,
                         closeHover ? sf::Color(120, 42, 46) : sf::Color(32, 32, 40),
                         closeHover ? sf::Color(220, 92, 96) : COL_BORDER, 1.1f);
    float xw = bodyFont.getTextWidth("X", 0.16f);
    bodyFont.drawText(window, "X", {closeRect.left + closeRect.width / 2.f - xw / 2.f, closeRect.top + 7.f},
                      0.16f, closeHover ? sf::Color(255, 230, 230) : COL_TEXT);

    sf::FloatRect selector(cardX + 205.f, cardY + 88.f, cardW - 245.f, 46.f);
    ui::drawRoundedPanel(window, selector, 6.f, sf::Color(28, 28, 36), COL_BORDER, 1.2f);

    sf::FloatRect selectorLeft(cardX + 215.f, cardY + 92.f, 38.f, 38.f);
    sf::FloatRect selectorRight(cardX + cardW - 78.f, cardY + 92.f, 38.f, 38.f);
    drawArrow(selectorLeft.left + selectorLeft.width / 2.f, selectorLeft.top + selectorLeft.height / 2.f,
              false, selectorLeft.contains(mp) ? 1.18f : 1.0f, selectorLeft.contains(mp));
    drawArrow(selectorRight.left + selectorRight.width / 2.f, selectorRight.top + selectorRight.height / 2.f,
              true, selectorRight.contains(mp) ? 1.18f : 1.0f, selectorRight.contains(mp));

    std::string selectedLabel = "No Sets Found";
    if (!pieceSets.empty()) {
        selectedLabel = draftCustomizePieces ? "Customize" : pieceSets[draftSetIndex].displayName;
    }
    float selScale = 0.20f;
    float selW = bodyFont.getTextWidth(selectedLabel, selScale);
    bodyFont.drawText(window, selectedLabel,
                      {selector.left + selector.width / 2.f - selW / 2.f, selector.top + 14.f},
                      selScale, COL_TEXT);

    float listY = cardY + 164.f;
    float rowH = 58.f;
    float rowGap = 8.f;

    auto drawPreview = [&](char piece, int setIdx, float cx, float cy, float maxSize) {
        if (!pieceSets.empty()) {
            setIdx = std::clamp(setIdx, 0, (int)pieceSets.size() - 1);
            auto it = pieceSets[setIdx].textures.find(piece);
            if (it != pieceSets[setIdx].textures.end()) {
                sf::Sprite sprite(it->second);
                float scale = textureFitScale(it->second, maxSize);
                sprite.setScale(scale, scale);
                sprite.setOrigin(it->second.getSize().x / 2.f, it->second.getSize().y / 2.f);
                sprite.setPosition(cx, cy);
                window.draw(sprite);
                return;
            }
        }

        ui::drawRoundedPanel(window, {cx - maxSize / 2.f, cy - maxSize / 2.f, maxSize, maxSize}, 5.f,
                             sf::Color(26, 26, 32), sf::Color(72, 72, 84), 1.f);
    };

    for (int i = 0; i < (int)PIECE_TYPES.size(); ++i) {
        char pieceType = PIECE_TYPES[i];
        float rowY = listY + i * (rowH + rowGap);
        sf::FloatRect rowRect(cardX + 28.f, rowY, cardW - 56.f, rowH);
        ui::drawRoundedPanel(window, rowRect, 6.f, sf::Color(26, 26, 34), sf::Color(54, 54, 66), 1.f);

        int setIdx = draftCustomizePieces
            ? (draftCustomSetIndex.count(pieceType) ? draftCustomSetIndex[pieceType] : draftSetIndex)
            : draftSetIndex;

        drawPreview(pieceType, setIdx, rowRect.left + 52.f, rowRect.top + rowH / 2.f, 44.f);
        drawPreview((char)std::tolower((unsigned char)pieceType), setIdx, rowRect.left + 105.f, rowRect.top + rowH / 2.f, 44.f);

        bodyFont.drawText(window, pieceTypeName(pieceType), {rowRect.left + 145.f, rowRect.top + 18.f},
                          0.17f, COL_TEXT);

        std::string rowSet = pieceSets.empty() ? "Fallback" : pieceSets[std::clamp(setIdx, 0, (int)pieceSets.size() - 1)].displayName;
        if (draftCustomizePieces) {
            sf::FloatRect leftRect(cardX + cardW - 255.f, rowY + 11.f, 34.f, 34.f);
            sf::FloatRect rightRect(cardX + cardW - 55.f, rowY + 11.f, 34.f, 34.f);
            drawArrow(leftRect.left + leftRect.width / 2.f, leftRect.top + leftRect.height / 2.f,
                      false, leftRect.contains(mp) ? 1.15f : 1.0f, leftRect.contains(mp));
            drawArrow(rightRect.left + rightRect.width / 2.f, rightRect.top + rightRect.height / 2.f,
                      true, rightRect.contains(mp) ? 1.15f : 1.0f, rightRect.contains(mp));

            float sw = bodyFont.getTextWidth(rowSet, 0.15f);
            bodyFont.drawText(window, rowSet, {cardX + cardW - 155.f - sw / 2.f, rowRect.top + 20.f},
                              0.15f, COL_SUBTEXT);
        }
    }

    sf::FloatRect cancelRect(cardX + cardW - 248.f, cardY + cardH - 54.f, 104.f, 36.f);
    sf::FloatRect applyRect(cardX + cardW - 132.f, cardY + cardH - 54.f, 104.f, 36.f);
    drawButton("CANCEL", cancelRect, cancelRect.contains(mp) ? 1.f : 0.f);
    drawButton("APPLY", applyRect, applyRect.contains(mp) ? 1.f : 0.f);
}
