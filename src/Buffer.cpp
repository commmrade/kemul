#include "Buffer.hpp"

TermBuffer::TermBuffer() {

}

TermBuffer::~TermBuffer() {

}

void TermBuffer::push_str(std::string str) {
    buffer.emplace_back(std::move(str));
}
void TermBuffer::add_str(std::string str) {
    buffer.back() += std::move(str);
}