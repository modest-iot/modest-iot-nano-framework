#ifndef FAKE_BLE2902_H
#define FAKE_BLE2902_H

class BLEDescriptor {
public:
    virtual ~BLEDescriptor() = default;
};

class BLE2902 : public BLEDescriptor {
public:
    BLE2902() = default;
    ~BLE2902() override = default;
};

#endif // FAKE_BLE2902_H
