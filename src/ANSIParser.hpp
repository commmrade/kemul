#include "Application.hpp"
#include "Buffer.hpp"
#include <utf8cpp/utf8.h>

class AnsiParser {
    private:
        Application& application;
        Cell current_cell;

        // state machine
        enum class GeneralState { TEXT, ESCAPE, CSI };
        GeneralState state = GeneralState::TEXT;

    public:
        AnsiParser(Application& app);

        void parse(const std::string& text);

    private:
        // Parse CSI parameters "1;31" -> 1, 31
        std::vector<int> parse_params(const std::string& csi_sequence);

        // Handle CSI commands
        void handle_CSI(char command, std::vector<int> params); // No need for vec& params
    };
