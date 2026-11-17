
#ifndef BX_TYPEINFO_H
#define BX_TYPEINFO_H

class BxTypeInfo {
public:
    BxTypeInfo(const char* name);

    const char* name() const { return _name; }

    bool canCatch(const BxTypeInfo* other) const;

private:
    const char* _name;
};

#endif
