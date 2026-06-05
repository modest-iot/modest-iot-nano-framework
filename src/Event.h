#ifndef EVENT_H
#define EVENT_H

class Event {
public:
    int identifier;

    explicit Event(int eventIdentifier) : identifier(eventIdentifier) {}

    bool operator==(const Event& other) const {
        return this->identifier == other.identifier;
    }
};

#endif // EVENT_H