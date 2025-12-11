#include "server.hpp"

void handle_pass(int client_fd, const std::string &cmd, ServerEnv *env)
{
    Client &client = env->getClient(client_fd);

    if (client.isAuthenticated())
    {
        send_message(client_fd, "Error: You are already authenticated\r\n");
        return;
    }

    size_t space_pos = cmd.find(' ');
    if (space_pos == std::string::npos)
    {
        send_message(client_fd, "Error: No password given\r\n");
        return;
    }

    std::string password = trim(cmd.substr(space_pos + 1));

    if (password == env->getPass())
    {
        client.setPassGiven(true);
        std::cout << "Client " << client_fd - 3 << " provided correct password" << std::endl;
    }
    else
    {
        send_message(client_fd, "Error: Invalid password\r\n");
        disconnect_client(client_fd, env);
    }
}

void handle_nick(int client_fd, std::string &cmd, ServerEnv *env)
{
    Client &client = env->getClient(client_fd);

    size_t space_pos = cmd.find(' ');
    if (space_pos == std::string::npos)
    {
        send_message(client_fd, "Error: No nickname given\r\n");
        return;
    }

    std::string nickname = trim(cmd.substr(space_pos + 1));

    if (nickname.empty())
    {
        send_message(client_fd, "Error: No nickname given\r\n");
        return;
    }

    if (nickname.length() > 9)
    {
        send_message(client_fd, "432 * " + nickname + " :Erroneous nickname (too long)\r\n");
        return;
    }

    if (!std::isalpha(nickname[0]))
    {
        send_message(client_fd, "432 * " + nickname + " :Erroneous nickname (must start with letter)\r\n");
        return;
    }

    for (size_t i = 0; i < nickname.length(); i++)
    {
        char c = nickname[i];
        if (!std::isalnum(c) && c != '-' && c != '_')
        {
            send_message(client_fd, "432 * " + nickname + " :Erroneous nickname (invalid characters)\r\n");
            return;
        }
    }

    for (std::map<int, Client>::iterator it = env->getClients().begin(); it != env->getClients().end(); ++it)
    {
        if (it->first != client_fd && it->second.getNickname() == nickname)
        {
            send_message(client_fd, "433 * " + nickname + " :Nickname is already in use\r\n");
            return;
        }
    }

    client.setNickname(nickname);
    client.setNickGiven(true);

    std::cout << "Client " << client_fd - 3<< " set nickname to: " << nickname << std::endl;
}

void handle_user(int client_fd, const std::string &cmd, ServerEnv *env)
{
    Client &client = env->getClient(client_fd);

    if (!client.isPassGiven())
    {
        send_message(client_fd, "Error: Send PASS first\r\n");
        return;
    }

    if (!client.isNickGiven())
    {
        send_message(client_fd, "Error: Send NICK first\r\n");
        return;
    }

    std::vector<std::string> parts = split_string(cmd, ' ');
    if (parts.size() < 5)
    {
        send_message(client_fd, "Error: Invalid USER (USER <username> <mode> <unused> :<realname>)\r\n");
        return;
    }

    std::string username = parts[1];

    if (username.empty() || username.length() > 16)
    {
        send_message(client_fd, "Error: Invalid username (empty or too long)\r\n");
        return;
    }

    if (!std::isalpha(username[0]))
    {
        send_message(client_fd, "Error: Invalid username (must start with letter)\r\n");
        return;
    }

    for (size_t i = 0; i < username.length(); i++)
    {
        char c = username[i];
        if (!std::isalnum(c) && c != '-' && c != '_')
        {
            send_message(client_fd, "Error: Invalid username (invalid characters)\r\n");
            return;
        }
    }

    for (std::map<int, Client>::iterator it = env->getClients().begin(); it != env->getClients().end(); ++it)
    {
        if (it->first != client_fd && it->second.getUsername() == username)
        {
            send_message(client_fd, "Error: Username already in use\r\n");
            return;
        }
    }

    client.setUsername(username);

    size_t colon_pos = cmd.find(':');
    if (colon_pos != std::string::npos)
    {
        std::string realname = cmd.substr(colon_pos + 1);
        
        if (realname.length() > 50)
        {
            send_message(client_fd, "Error: Realname too long (max 50 characters)\r\n");
            return;
        }
        
        for (size_t i = 0; i < realname.length(); i++)
        {
            if (!std::isprint(static_cast<unsigned char>(realname[i])))
            {
                send_message(client_fd, "Error: Realname contains non-printable characters\r\n");
                return;
            }
        }
        
        client.setRealname(realname);
    }

    client.setUserGiven(true);
    client.setAuthenticated(true);

    std::cout << "Client " << client_fd - 3 << " authenticated as "
              << client.getNickname() << " (" << client.getUsername() << ")" << std::endl;    std::string welcome = ":server 001 " + client.getNickname() + 
                          " :Welcome to the IRC Network, " + client.getNickname() + "\r\n";
    send_message(client_fd, welcome);
}