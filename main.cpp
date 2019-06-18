#include <iostream>
#include <cassert>
#include <sstream>
#include <functional>


namespace engine {
    enum Token {
        addition,
        subtraction,
        multiplication,
        division,
        opened_parentheses,
        closed_parentheses,
        number,
        eof,
    };

    class Tokenizer {
    public:
        Token current_token = engine::number;
        char current_char = 1;
        double number = 0;

        int position = 0;
        std::string input;

        void next_char() {
            char symbol = this->input[this->position];
            this->position++;

            this->current_char = symbol < 0 ? '\0' : symbol;
        }

        void set_input(std::string _input) {
            position = 0;
            current_char = 1;
            current_token = engine::number;
            this->input = std::move(_input);

            next_char();
            next_token();
        }

        void next_token() {
            while (this->current_char == ' ') {
                this->next_char();
            }

            switch (this->current_char) {
                case '\0':
                    this->current_token = engine::eof;
                    return;
                case '+':
                    this->next_char();
                    this->current_token = engine::addition;
                    return;
                case '-':
                    this->next_char();
                    this->current_token = engine::subtraction;
                    return;
                case '*':
                    this->next_char();
                    this->current_token = engine::multiplication;
                    return;
                case '/':
                    this->next_char();
                    this->current_token = engine::division;
                    return;
                case '(':
                    this->next_char();
                    this->current_token = engine::opened_parentheses;
                    return;
                case ')':
                    this->next_char();
                    this->current_token = engine::closed_parentheses;
                    return;
            }


            if (isdigit(this->current_char) || this->current_char =='.') {
                bool is_decimal = false;
                std::stringstream string_builder;

                while (isdigit(this->current_char) || (!is_decimal && this->current_char == '.'))
                {
                    string_builder << this->current_char;
                    is_decimal = this->current_char == '.';
                    next_char();
                }

                this->number = strtod(string_builder.str().c_str(), nullptr);
                this->current_token = engine::number;
                return;
            }

            std::cout << "Current char : |" << current_char << "|" << std::endl;
            throw std::logic_error(&"Not supported type of operator: " [ current_char]);
        }

        explicit Tokenizer(std::string input) {
            this->input = std::move(input);

            next_char();
            next_token();
        }

        Tokenizer() = default;
    };

    class Node {
        public:
        virtual double eval() = 0;
    };

    class NumberNode : public Node {
    public:
        double number;

        explicit NumberNode(double number) {
            this->number = number;
        }

        double eval() override {
            return number;
        }
    };

    class BinaryOperationNode : public Node {
    public:
        Node *left;
        Node *right;
        std::function<double(double, double)> operation;

        BinaryOperationNode(Node *left, Node *right, std::function<double(double, double)> operation) {
            this->left = left;
            this->right = right;
            this->operation = std::move(operation);
        }

        double eval() override {
            auto left_value = left->eval();
            auto right_value = right->eval();

            return operation(left_value, right_value);
        }
    };

    class UnaryOperationNode : public Node {
    public:
        Node *right;
        std::function<double(double)> operation;

        UnaryOperationNode(Node *right, std::function<double(double)> operation) {
            this->right=right;
            this->operation = std::move(operation);
        }

        double eval() override {
            auto right_value = right->eval();

            return operation(right_value);
        }
    };

    class Parser {
    public:
        Tokenizer *tokenizer;

        explicit Parser(Tokenizer *tokenizer) {
            this->tokenizer = tokenizer;
        }

        Node* parse_expression() {
            Node *expression = parse_addition_and_subtraction_operators();

            if (tokenizer->current_token != engine::eof)
                throw std::logic_error("Not understandable expression");

            return expression;
        }

        Node* parse_addition_and_subtraction_operators() {
            auto left_leaf = parse_multiplication_and_division_operators();

            while (true) {
                std::function<double(double, double)> operation = nullptr;
                if (tokenizer->current_token == engine::addition) {
                    operation = [](double a, double b) -> double { return a + b; };
                }
                else if (tokenizer->current_token == engine::subtraction) {
                    operation = [](double a, double b) -> double { return a - b; };
                }

                if (operation == nullptr)
                    return left_leaf;

                tokenizer->next_token();

                auto right_leaf = parse_multiplication_and_division_operators();
                left_leaf = new BinaryOperationNode(left_leaf, right_leaf, operation);
            }
        }

        Node* parse_multiplication_and_division_operators() {
            auto left = parse_unary_operator();

            while (true) {
                std::function<double(double, double)> operation = nullptr;

                if (tokenizer->current_token == engine::multiplication) {
                    operation = [](double a, double b) -> double { return a * b; };
                }
                else if (tokenizer->current_token == engine::division) {
                    operation = [](double a, double b) -> double { return a / b; };
                }

                if (operation == nullptr)
                    return left;
                tokenizer->next_token();

                auto right = parse_unary_operator();

                left = new BinaryOperationNode(left, right, operation);
            }
        }

        Node* parse_unary_operator() {
            while (true) {
                if (tokenizer->current_token == engine::addition) {
                    tokenizer->next_token();
                    continue;
                }

                if (tokenizer->current_token == engine::subtraction) {
                    tokenizer->next_token();

                    auto right = parse_unary_operator();
                    return new UnaryOperationNode(right, [](double a) -> double { return -a; });
                }
                return parse_leaf();
            }
        }

        Node* parse_leaf() {
            if (tokenizer->current_token == engine::number) {
                auto *node = new NumberNode(tokenizer->number);
                tokenizer->next_token();
                return node;
            }

            if (tokenizer->current_token == engine::opened_parentheses) {
                tokenizer->next_token();

                auto node = parse_addition_and_subtraction_operators();

                if (tokenizer->current_token != engine::closed_parentheses)
                    throw std::logic_error("Missing parentheses");
                tokenizer->next_token();

                return node;
            }

            throw std::logic_error(&"Unexpect token: " [ tokenizer->current_token]);
        }
    };
}



int main(int argc, char *argv[]) {
    std::string str;
    std::cin >> str;

    // str = "10 + 20";
    auto tokenizer = new engine::Tokenizer(str);
    auto parser = new engine::Parser(tokenizer);

    std::cout << parser->parse_expression() << std::endl;

    assert(parser->parse_expression()->eval() == 30);
    parser->tokenizer->set_input("10 - 20");
    assert(parser->parse_expression()->eval() == -10 && "2");
    parser->tokenizer->set_input("10 + 20 - 40 + 100");
    assert(parser->parse_expression()->eval() == 90 &&"3");
    parser->tokenizer->set_input("-10");
    assert(parser->parse_expression()->eval() == -10 && "4");
    parser->tokenizer->set_input("+10");
    assert(parser->parse_expression()->eval() == 10 &&"5");
    parser->tokenizer->set_input("--10");
    assert(parser->parse_expression()->eval() == 10 && "6");
    parser->tokenizer->set_input("--++-+-10");
    assert(parser->parse_expression()->eval() == 10 && "7");
    parser->tokenizer->set_input("10 + -20 - +30");
    assert(parser->parse_expression()->eval() == -40 && "8");
    parser->tokenizer->set_input("10 * 20");
    assert(parser->parse_expression()->eval() == 200 && "9");
    parser->tokenizer->set_input("10 / 20");
    assert(parser->parse_expression()->eval() == 0.5 && "10");
    parser->tokenizer->set_input("10 * 20 / 50");
    assert(parser->parse_expression()->eval() == 4 && "11");
    parser->tokenizer->set_input("10 + 20 * 30");
    assert(parser->parse_expression()->eval() == 610 && "12");
    parser->tokenizer->set_input("(10 + 20) * 30");
    assert(parser->parse_expression()->eval() == 900 && "13");
    parser->tokenizer->set_input("-(10 + 20) * 30");
    assert(parser->parse_expression()->eval() == -900 && "14");

    std::cout << parser->parse_expression()->eval() << std::endl;
    return 0;
}