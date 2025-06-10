#pragma once
#include <cereal/access.hpp>
#include <cstddef>
#include <glm/fwd.hpp>

namespace Coffee {

    class UUID
    {
    public:
        static const UUID null;

    public:
        UUID();
        UUID(uint64_t uuid);
        UUID(const UUID&) = default;

        operator uint64_t() const { return m_UUID; }
    private:
        friend class cereal::access;

        template <class Archive> void serialize(Archive& archive, std::uint32_t const version)
        {
            archive(m_UUID);
        }
    private:
        uint64_t m_UUID;
    };

    inline const UUID UUID::null = UUID(0);

}

namespace std {
    template <typename T> struct hash;

    template<>
    struct hash<Coffee::UUID>
    {
        std::size_t operator()(const Coffee::UUID& uuid) const
        {
            return (glm::uint64_t)uuid;
        }
    };

}