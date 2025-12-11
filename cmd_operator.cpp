#include "server.hpp"

void handle_kick(int client_fd, const std::string &cmd, ServerEnv *env)
{
    Client &client = env->getClient(client_fd);

    std::vector<std::string> parts = split_string(cmd, ' ');
    if (parts.size() < 3)
    {
        send_message(client_fd, "Error: Usage: KICK <channel> <user> :[reason]\r\n");
        return;
    }

    std::string channel_name = parts[1];
    std::string target_nick = parts[2];

    if (!is_client_operator(client_fd, channel_name, env))
    {
        send_message(client_fd, "Error: You are not channel operator\r\n");
        return;
    }

    int target_fd = -1;
    for (std::map<int, Client>::iterator it = env->getClients().begin(); it != env->getClients().end(); ++it)
    {
        if (it->second.getNickname() == target_nick)
        {
            target_fd = it->first;
            break;
        }
    }

    if (target_fd == -1)
    {
        send_message(client_fd, "Error: No such user\r\n");
        return;
    }

    if (!is_client_in_channel(target_fd, channel_name, env))
    {
        send_message(client_fd, "Error: User is not in channel\r\n");
        return;
    }

    Channel &chan = env->getChannel(channel_name);
    
    // Check if admin is trying to kick themselves
    if (target_fd == client_fd && chan.isAdmin(client_fd))
    {
        // Check if they are the only admin
        if (chan.getAdmins().size() == 1)
        {
            send_message(client_fd, "Error: You are the only operator in this channel. Promote another user to operator first (MODE " + channel_name + " +o <nickname>)\r\n");
            return;
        }
    }
    
    // Remove user from channel, admin list, AND invite list
    chan.removeClient(target_fd);
    chan.removeAdmin(target_fd);
    chan.removeInvited(target_fd);

    std::string kick_msg = ":" + client.getNickname() + " KICK " + channel_name +
                           " " + target_nick + " :Kicked by operator\r\n";
    broadcast_to_channel(kick_msg, channel_name, env);
    send_message(target_fd, kick_msg);

    std::cout << client.getNickname() << " kicked " + target_nick << " from " << channel_name << std::endl;
}

void handle_invite(int client_fd, const std::string &cmd, ServerEnv *env)
{
    Client &client = env->getClient(client_fd);

    std::vector<std::string> parts = split_string(cmd, ' ');
    if (parts.size() < 3)
    {
        send_message(client_fd, "Error: Usage: INVITE <user> <channel>\r\n");
        return;
    }

    std::string target_nick = parts[1];
    std::string channel_name = parts[2];

    if (!is_client_operator(client_fd, channel_name, env))
    {
        send_message(client_fd, "Error: You're not channel operator\r\n");
        return;
    }

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
        send_message(client_fd, "Error: No such user\r\n");
        return;
    }

    Channel &chan = env->getChannel(channel_name);
    if (!chan.isInvited(target_fd))
        chan.addInvited(target_fd);

    std::string invite_msg = ":" + client.getNickname() + " INVITE " + target_nick +
                             " " + channel_name + "\r\n";
    send_message(target_fd, invite_msg);

    std::cout << client.getNickname() << " invited " << target_nick << " to " << channel_name << std::endl;
}

void handle_topic(int client_fd, const std::string &cmd, ServerEnv *env)
{
    Client &client = env->getClient(client_fd);

    size_t first_space = cmd.find(' ');
    if (first_space == std::string::npos)
    {
        send_message(client_fd, "Error: Usage: TOPIC <channel> [:<new topic>]\r\n");
        return;
    }

    size_t colon_pos = cmd.find(':', first_space);
    std::string channel_name;

    if (colon_pos != std::string::npos)
        channel_name = trim(cmd.substr(first_space + 1, colon_pos - first_space - 1));
    else
        channel_name = trim(cmd.substr(first_space + 1));

    if (!env->hasChannel(channel_name))
    {
        send_message(client_fd, "Error: No such channel");
        return;
    }

    Channel &chan = env->getChannel(channel_name);

    if (!is_client_in_channel(client_fd, channel_name, env))
    {
        send_message(client_fd, "Error: You're not in that channel\r\n");
        return;
    }

    if (colon_pos == std::string::npos)
    {
        if (chan.getTopic().empty())
        {
            std::string msg = ":server 331 " + client.getNickname() + " " +
                              channel_name + " :No topic is set\r\n";
            send_message(client_fd, msg);
        }
        else
        {
            std::string msg = ":server 332 " + client.getNickname() + " " +
                              channel_name + " :" + chan.getTopic() + "\r\n";
            send_message(client_fd, msg);
        }
        return;
    }

    if (chan.isTopicLocked() && !is_client_operator(client_fd, channel_name, env))
    {
        send_message(client_fd, "Error: Topic is locked to operators only\r\n");
        return;
    }

    std::string new_topic = cmd.substr(colon_pos + 1);
    chan.setTopic(new_topic);
    std::string topic_msg = ":" + client.getNickname() + " Topic " + channel_name +
                            " :" + new_topic + "\r\n";
    broadcast_to_channel(topic_msg, channel_name, env);

    std::cout << client.getNickname() << " changed topic of " << channel_name << " to: " << new_topic << std::endl;
}

void handle_mode(int client_fd, const std::string &cmd, ServerEnv *env)
{
    Client &client = env->getClient(client_fd);

    std::vector<std::string> parts = split_string(cmd, ' ');
    if (parts.size() < 2)
    {
        send_message(client_fd, "Error: Usage: MODE <channel> [+/-modes] [params]\r\n");
        return;
    }

    std::string channel_name = parts[1];

    if (!env->hasChannel(channel_name))
    {
        send_message(client_fd, "403 " + client.getNickname() + " " + channel_name + " :No such channel\r\n");
        return;
    }

    Channel &chan = env->getChannel(channel_name);

    // If no mode string provided, return current channel modes
    if (parts.size() == 2)
    {
        std::string mode_string = "+";
        std::string mode_params;

        if (chan.isInviteOnly())
            mode_string += "i";
        if (chan.isTopicLocked())
            mode_string += "t";
        if (!chan.getPassword().empty())
        {
            mode_string += "k";
            // Only show password to operators
            if (chan.isAdmin(client_fd))
                mode_params += " " + chan.getPassword();
            else
                mode_params += " *";
        }
        if (chan.getUserLimit() > 0)
        {
            mode_string += "l";
            mode_params += " " + int_to_string(chan.getUserLimit());
        }

        // If no modes set, just send +
        if (mode_string == "+")
            mode_string = "+";

        send_message(client_fd, ":server 324 " + client.getNickname() + " " + 
                     channel_name + " " + mode_string + mode_params + "\r\n");
        return;
    }

    std::string modes = parts[2];

    if (!is_client_operator(client_fd, channel_name, env))
    {
        send_message(client_fd, "Error: You're not channel operator\r\n");
        return;
    }

    bool adding = true;
    std::string mode_params;

    for (size_t i = 0; i < modes.length(); i++)
    {
        char mode = modes[i];

        if (mode == '+')
            adding = true;
        else if (mode == '-')
            adding = false;
        else if (mode == 'i')
        {
            chan.setInviteOnly(adding);
            std::cout << "Channel " << channel_name << " invite-only: " << adding << std::endl;
        }
        else if (mode == 't')
        {
            chan.setTopicLocked(adding);
            std::cout << "Channel " << channel_name << " topic locked: " << adding << std::endl;
        }
        else if (mode == 'k')
        {
            if (adding && parts.size() >= 4)
            {
                chan.setPassword(parts[3]);
                mode_params += " " + parts[3];
                std::cout << "Channel " << channel_name << " password set" << std::endl;
            }
            else if (!adding)
            {
                chan.setPassword("");
                std::cout << "Channel " << channel_name << " password removed" << std::endl;
            }
        }
        else if (mode == 'l')
        {
            if (adding && parts.size() >= 4)
            {
                chan.setUserLimit(std::atoi(parts[3].c_str()));
                mode_params += " " + parts[3];
                std::cout << "Channel " << channel_name << " limit: " << chan.getUserLimit() << std::endl;
            }
            else if (!adding)
            {
                chan.setUserLimit(0);
                std::cout << "Channel " << channel_name << " limit removed" << std::endl;
            }
        }
        else if (mode == 'o')
        {
            if (parts.size() >= 4)
            {
                std::string target_nick = parts[3];

                int target_fd = -1;
                for (std::map<int, Client>::iterator it = env->getClients().begin();
                     it != env->getClients().end(); ++it)
                {
                    if (it->second.getNickname() == target_nick)
                    {
                        target_fd = it-> first;
                        break;
                    }
                }

                if (target_fd != -1)
                {
                    if(adding) 
                    {
                        if(!chan.isAdmin(target_fd))
                            chan.addAdmin(target_fd);
                        mode_params += " " + target_nick;
                        std::cout << target_nick << " given operator in " << channel_name << std::endl;
                    }
                    else
                    {
                        chan.removeAdmin(target_fd);
                        mode_params += " " + target_nick;
                        std::cout << target_nick << " removed from operators in " << channel_name << std::endl;
                    }
                }
            }
        }
    }

    std::string mode_msg = ":" + client.getNickname() + " MODE " + channel_name + " " + modes + mode_params + "\r\n";
    broadcast_to_channel(mode_msg, channel_name, env);
}