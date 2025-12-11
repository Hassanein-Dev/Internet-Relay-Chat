#include "server.hpp"

void handle_whois(int client_fd, const std::string &cmd, ServerEnv *env)
{
    Client &client = env->getClient(client_fd);
    std::vector<std::string> parts = split_string(cmd, ' ');

    if (parts.size() < 2)
    {
        send_message(client_fd, "461 " + client.getNickname() + " WHOIS :Not enough parameters\r\n");
        return;
    }

    std::string target_nick = parts[1];
    
    int target_fd = -1;
    for (std::map<int, Client>::iterator it = env->getClients().begin(); 
         it != env->getClients().end(); ++it)
    {
        if (it->second.getNickname() == target_nick)
        {
            target_fd = it->first;
            break;
        }
    }

    if (target_fd == -1)
    {
        send_message(client_fd, "401 " + client.getNickname() + " " + target_nick + " :No such nick\r\n");
        return;
    }

    Client &target = env->getClient(target_fd);

    send_message(client_fd, ":server 311 " + client.getNickname() + " " + 
                 target_nick + " " + target.getUsername() + " localhost * :" + 
                 target.getRealname() + "\r\n");

    send_message(client_fd, ":server 312 " + client.getNickname() + " " + 
                 target_nick + " server :IRC Server\r\n");

    std::string channels_list;
    for (std::map<std::string, Channel>::iterator it = env->getChannels().begin();
         it != env->getChannels().end(); ++it)
    {
        if (it->second.hasClient(target_fd))
        {
            if (!channels_list.empty())
                channels_list += " ";
            
            if (it->second.isAdmin(target_fd))
                channels_list += "@";
            
            channels_list += it->first;
        }
    }
    
    if (!channels_list.empty())
    {
        send_message(client_fd, ":server 319 " + client.getNickname() + " " + 
                     target_nick + " :" + channels_list + "\r\n");
    }

    send_message(client_fd, ":server 318 " + client.getNickname() + " " + 
                 target_nick + " :End of WHOIS list\r\n");

    std::cout << client.getNickname() << " requested WHOIS for " << target_nick << std::endl;
}