
#ifndef BX_TRIGGERFILE_H
#define BX_TRIGGERFILE_H

// *************************************************************************************************

class BxTriggerFile {
public:
    BxTriggerFile();
    ~BxTriggerFile();
    BxTriggerFile(const BxTriggerFile&) = delete;
    BxTriggerFile& operator=(const BxTriggerFile&) = delete;

    void trigger();
    void flush();
    void close();

public:
    int fd;

private:
    int fdOut;
};

// *************************************************************************************************

#endif
