#include <string>

enum ShredingerStatus {
    Null = 0,
    Undefined = 1,
    Alive = 2,
    Dead = 3
};

ShredingerStatus min_shredinger_status = Undefined;
ShredingerStatus max_shredinger_status = Dead;

std::string pipe_name = "/tmp/shredingerpipe";
std::string semaphore_name = "/ShredingerStartSem";
