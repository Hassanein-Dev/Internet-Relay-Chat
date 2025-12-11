#include "server.hpp"

static ServerEnv *g_env = NULL;

void signal_handler(int signum)
{
    (void)signum;
    if (g_env)
    {
        cleanup_server(g_env);
        std::cout << "\nServer shutdown complete." << std::endl;
        exit(0);
    }
}

void validate_argument(int argc, char **argv)
{
    if(argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        exit(1);
    }

    std::string str_port = argv[1];
    for(size_t i = 0; i < str_port.length(); i++)
    {
        if(!isdigit(str_port[i]))
        {
            std::cerr << "Error: port must be a number!" << std::endl;
            exit(1);
        }
    }

    int port = atoi(argv[1]);
    if(port < 1024 || port > 49151)
    {
        std::cerr << "port number should be between 1024 and 49151" << std::endl;
        exit(1);
    }

    std::string pass = argv[2];
    for(size_t i = 0; i < pass.length(); i++)
    {
        if(pass[i] == ' ' || pass[i] == '\t')
        {
            std::cerr << "Error: space found in password!" << std::endl;
            exit(1);
        }
    }
}

int main(int ac, char **av)
{
    validate_argument(ac, av);

    ServerEnv env;
    env.setPort(std::atoi(av[1]));
    env.setPassword(av[2]);

    env.setServerFd(create_server_socket(env.getPort()));
    
    // Setup signal handlers
    g_env = &env;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::cout << "waiting for Connections :)" << std::endl;

    server_loop(&env);

    cleanup_server(&env);
    return 0;
}