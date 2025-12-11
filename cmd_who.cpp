#include "server.hpp"

void handle_who(int client_fd, const std::string &cmd, ServerEnv *env)
{
    Client &client = env->getClient(client_fd);
    std::vector<std::string> parts = split_string(cmd, ' ');

    if (parts.size() < 2)
    {
        send_message(client_fd, "461 " + client.getNickname() + " WHO :Not enough parameters\r\n");
        return;
    }

    std::string channel_name = parts[1];

    if (!env->hasChannel(channel_name))
    {
        send_message(client_fd, "403 " + client.getNickname() + " " + channel_name + " :No such channel\r\n");
        return;
    }

    Channel &channel = env->getChannel(channel_name);

    if (!channel.hasClient(client_fd))
    {
        send_message(client_fd, "442 " + client.getNickname() + " " + channel_name + " :You're not on that channel\r\n");
        return;
    }

    const std::vector<int> &clients = channel.getClients();
    for (size_t i = 0; i < clients.size(); i++)
    {
        int user_fd = clients[i];
        Client &user = env->getClient(user_fd);
        
        // H = Here (not away), G = Gone (away)
        std::string status = "H";
        
        if (channel.isAdmin(user_fd))
            status += "@";
        
        // 352 format: <nick> <channel> <user> <host> <server> <nick> <status> :<hopcount> <realname>
        std::string who_reply = ":server 352 " + client.getNickname() + " " + channel_name + " " +
                               user.getUsername() + " localhost server " +
                               user.getNickname() + " " + status + " :0 " +
                               user.getRealname() + "\r\n";
        
        send_message(client_fd, who_reply);
    }

    send_message(client_fd, ":server 315 " + client.getNickname() + 
                " " + channel_name + " :End of WHO list\r\n");
}
