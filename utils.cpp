#include "server.hpp"

void send_message(int fd, const std::string &msg)
{
    send(fd, msg.c_str(), msg.length(), MSG_NOSIGNAL);
}

std::string trim(const std::string &str)
{
    size_t start = 0;
    size_t end = str.length();

    while(start < end && std::isspace(str[start]))
        start++;
    
    while(end > start && std::isspace(str[end - 1]))
        end--;
    
    return str.substr(start, end - start);
}

std::vector<std::string> split_string(const std::string &str, char delimiter)
{
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;

    while(std::getline(ss, item, delimiter))
    {
        if (!item.empty())
            result.push_back(item);
    }

    return result;
}

bool is_client_in_channel(int client_fd, const std::string &channel_name, ServerEnv *env)
{
    if(!env->hasChannel(channel_name))
        return false;
    
    Channel &chan = env->getChannel(channel_name);
    return chan.hasClient(client_fd);
}

void broadcast_to_channel(const std::string &msg, const std::string &channel_name, ServerEnv *env, int exclude_fd)
{
    if(!env->hasChannel(channel_name))
        return;
    
    Channel &chan = env->getChannel(channel_name);
    const std::vector<int> &clients = chan.getClients();

    for(size_t i = 0; i< clients.size(); i++)
    {
        int client_fd = clients[i];
        if(client_fd != exclude_fd)
            send_message(client_fd, msg);
    }
}

bool is_client_operator(int client_fd, const std::string &channel_name, ServerEnv *env)
{
    if(!env->hasChannel(channel_name))
        return false;
    
    Channel &chan = env->getChannel(channel_name);
    return chan.isAdmin(client_fd);
}