#include "server.hpp"

void handle_join(int client_fd, const std::string &cmd, ServerEnv *env)
{
    Client &client = env->getClient(client_fd);

    std::vector<std::string> parts = split_string(cmd, ' ');
    if(parts.size() < 2)
    {
        send_message(client_fd, "Error: Usage: JOIN <channel> [password]\r\n");
        return;
    }

    std::string channel_name = parts[1];
    std::string password = (parts.size() >= 3) ? parts[2] : "";

    if(channel_name.empty() || channel_name[0] != '#')
    {
        send_message(client_fd, "Error: Channel name must start with #\r\n");
        return;
    }

    bool channel_exists = env->hasChannel(channel_name);

    if(!channel_exists)
    {
        Channel new_channel;
        new_channel.setName(channel_name);
        new_channel.addClient(client_fd);
        new_channel.addAdmin(client_fd);

        env->addChannel(channel_name, new_channel);

        std::cout << client.getNickname() << " created channel " << channel_name << std::endl;

        std::string join_msg = ":" + client.getNickname() + "!~" + client.getUsername() +
                               "@" + client.getHostname() + " JOIN " + channel_name + "\r\n";
        send_message(client_fd, join_msg);

        std::string topic_msg = ":server 331 " + client.getNickname() + " " +
                                channel_name + " :No topic is set\r\n";
        send_message(client_fd, topic_msg);

        // Send NAMES list (who is in the channel)
        std::string names_list = "@" + client.getNickname();
        std::string names_msg = ":server 353 " + client.getNickname() + " = " +
                                channel_name + " :" + names_list + "\r\n";
        send_message(client_fd, names_msg);

        std::string end_names = ":server 366 " + client.getNickname() + " " +
                                channel_name + " :End of NAMES list\r\n";
        send_message(client_fd, end_names);
    }
    else
    {
        Channel &chan = env->getChannel(channel_name);

        if(is_client_in_channel(client_fd, channel_name, env))
        {
            send_message(client_fd, "Error: you are already in this channel\r\n");
            return;
        }

        if(chan.isInviteOnly())
        {
            if(!chan.isInvited(client_fd))
            {
                send_message(client_fd, "Error: Channel is invited-only\r\n");
                return;
            }
        }

        if(!chan.getPassword().empty() && chan.getPassword() != password)
        {
            send_message(client_fd, "Error: Incorrect channel password\r\n");
            return;
        }

        if(chan.getUserLimit() > 0 && (int)chan.getClients().size() >= chan.getUserLimit())
        {
            send_message(client_fd, "Error: Channel is full\r\n");
            return;
        }

        chan.addClient(client_fd);

        std::cout << client.getNickname() << " joined channel " << channel_name << std::endl;
    
        std::string join_msg = ":" + client.getNickname() + "!~" + client.getUsername() + 
                                "@" + client.getHostname() + " JOIN " + channel_name + "\r\n";
        broadcast_to_channel(join_msg, channel_name, env);

        if(chan.getTopic().empty())
        {
            std::string topic_msg = ":server 331 " + client.getNickname() + " " +
                                    channel_name + " :No topic is set\r\n";
            send_message(client_fd, topic_msg);
        }
        else
        {
            std::string topic_msg = ":server 332 " + client.getNickname() + " " +
                                    channel_name + ":" + chan.getTopic() + "\r\n";
            send_message(client_fd, topic_msg);
        }

        // Send NAMES list (who is in the channel)
        std::string names_list;
        const std::vector<int> &clients = chan.getClients();
        for (size_t i = 0; i < clients.size(); i++)
        {
            if (env->hasClient(clients[i]))
            {
                if (!names_list.empty())
                    names_list += " ";
                
                // Add @ prefix for operators
                if (chan.isAdmin(clients[i]))
                    names_list += "@";
                
                names_list += env->getClient(clients[i]).getNickname();
            }
        }

        std::string names_msg = ":server 353 " + client.getNickname() + " = " +
                                channel_name + " :" + names_list + "\r\n";
        send_message(client_fd, names_msg);

        std::string end_names = ":server 366 " + client.getNickname() + " " +
                                channel_name + " :End of NAMES list\r\n";
        send_message(client_fd, end_names);
    }
}