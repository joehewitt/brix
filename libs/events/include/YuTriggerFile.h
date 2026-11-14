
#ifndef YU_TRIGGERFILE_H
#define YU_TRIGGERFILE_H

// *************************************************************************************************

class YuTriggerFile {
public:
    YuTriggerFile();
    ~YuTriggerFile();
    YuTriggerFile(const YuTriggerFile&) = delete;
    YuTriggerFile& operator=(const YuTriggerFile&) = delete;

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
