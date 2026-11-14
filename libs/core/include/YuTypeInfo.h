
#ifndef YU_TYPEINFO_H
#define YU_TYPEINFO_H

class YuTypeInfo {
public:
    YuTypeInfo(const char* name);

    const char* name() const { return _name; }

    bool canCatch(const YuTypeInfo* other) const;

private:
    const char* _name;
};

#endif
