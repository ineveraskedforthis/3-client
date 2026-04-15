#include <cstdint>
#include <string>

namespace item {
namespace kind {
constexpr inline uint8_t rat_body = 0;
constexpr inline uint8_t rat_fur = 1;
constexpr inline uint8_t rat_body_skinned = 2;
constexpr inline uint8_t rat_skeleton = 3;
constexpr inline uint8_t human_body = 4;
constexpr inline uint8_t human_body_skinned = 5;
constexpr inline uint8_t human_skeleton = 6;
}

const std::string names[7] {
                                "Rat body",
                                "Rat fur",
                                "Skinned rat body",
                                "Rat skeleton",
                                "Human body",
                                "Human body skinned",
                                "Human skeleton"
};
}