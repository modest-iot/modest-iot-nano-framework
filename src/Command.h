#ifndef COMMAND_H
#define COMMAND_H

class Command {
public:
    int identifier;

    explicit Command(int commandIdentifier) : identifier(commandIdentifier) {}

    bool operator==(const Command& other) const {
        return this->identifier == other.identifier;
    }
};

#endif // COMMAND_H