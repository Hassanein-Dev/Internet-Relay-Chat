#include "server.hpp"

void handle_privmsg(int client_fd, const std::string &cmd, ServerEnv *env)
{
    Client &client = env->getClient(client_fd);

    size_t first_space = cmd.find(' ');
    if(first_space == std::string::npos)
    {
        send_message(client_fd, "Error: No target given\r\n");
        return;
    }

    size_t colon_pos = cmd.find(':', first_space);
    if(colon_pos == std::string::npos)
    {
        send_message(client_fd, "Error: No message given\r\n");
        return;
    }

    std::string target = trim(cmd.substr(first_space + 1, colon_pos - first_space -1));
    std::string message = cmd.substr(colon_pos + 1);

    if(target.empty() || message.empty())
    {
        send_message(client_fd, "Error: Invalid PRIVMSG format\r\n");
        return;
    }

    if(target[0] == '#')
    {
        if(!env->hasChannel(target))
        {
            send_message(client_fd, "Error: No such channel\r\n");
            return;
        }

        if(!is_client_in_channel(client_fd, target, env))
        {
            send_message(client_fd,"Error: You're not in that channel\r\n");
            return;
        }

        std::string formatted_msg = ":" + client.getNickname() + "!~" + client.getUsername() +
                                    "@" + client.getHostname() + " PRIVMSG " + target + " :" + message + "\r\n";
        
        if(!handle_bot_command(client_fd, target, message, env))
        {
            broadcast_to_channel(formatted_msg, target, env, client_fd);
        }

        std::cout << client.getNickname() << " -> " << ": " << message << std::endl;
    }
    else
    {
        int target_fd = -1;
        for (std::map<int, Client>::iterator it = env->getClients().begin(); it != env->getClients().end(); ++it)
        {
            if(it->second.getNickname() == target)
            {
                target_fd = it->first;
                break;
            }
        }
        
        if(target_fd == -1)
        {
            send_message(client_fd, "Error: No such user\r\n");
            return;
        }

        std::string formatted_msg = ":" + client.getNickname() + "!~" + client.getUsername() +
                                    "@" + client.getHostname() + " PRIVMSG " + target + " :" + message + "\r\n";
        send_message(target_fd, formatted_msg);

        std::cout << client.getNickname() << " -> " << target << "(private): " << message << std::endl;
    }
}