#include <stdint.h>

struct circle_params_t {
	std::string ccca;
	std::string cccb;
	std::string cccc;
};

struct sender_params_t {
    std::string ccca;
    std::string cccb;
    std::string cccc;
    uint64_t    cnt;
};

struct origin_params_t {
    std::string origin;
    std::string ccca;
    std::string cccb;
    std::string cccc;
    uint64_t    cnt;
};

struct ram_params_t {
    std::string ccca;
    std::string cccb;
    std::string cccc;
    bool        aflag;
    bool        bflag;
    bool        cflag;
    uint64_t    pk = 0;
};




