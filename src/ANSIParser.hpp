#include "Application.hpp"
#include "Buffer.hpp"
#include <utf8cpp/utf8.h>

class AnsiParser {
    private:
        Application& application;      // Reference to the Application instance
        Cell current_cell;            // Current state of cell attributes
        // std::vector<Cell> cells;      // Buffer of parsed cells
    
        // State machine states
        enum class GeneralState { TEXT, ESCAPE, CSI };
        GeneralState state = GeneralState::TEXT;
    
    public:
        AnsiParser(Application& app);
    
        void parse(const std::string& text);

        void parse_input(const std::string& text);
    private:
        // Parse CSI parameters (e.g., "1;31" -> {1, 31})
        std::vector<int> parse_params(const std::string& csi_sequence);
    
        // Handle CSI commands (e.g., 'm' for SGR, 'H' for cursor position)
        void handle_CSI(char command, std::vector<int> params);
    };