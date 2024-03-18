#pragma once
#include <atomic>

typedef uint32_t uid_t;

class UniqueID
{
protected:
    inline static std::atomic<uid_t> curID = 0;
    uid_t id;
    
public:
    static constexpr uid_t unassigned = -1;
    
    UniqueID() : id(++curID) {}
    ~UniqueID() = default;

    // TODO: Not sure about this... Currently copy=newID and move=sameID.
    UniqueID(const UniqueID&)                      : id(++curID) {}
    UniqueID(UniqueID&& other) noexcept            : id(other.id) { other.id = -1; }
    UniqueID& operator=(const UniqueID&)           { id = ++curID; return *this; }
    UniqueID& operator=(UniqueID&& other) noexcept { id = other.id; other.id = -1; return *this; }
    
    const uid_t& GetID() const { return id; }
};
