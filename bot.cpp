#include "server.hpp"

bool handle_bot_command(int client_fd, const std::string &channel_name, const std::string &message, ServerEnv *env)
{
    if (message.find("!bot") != 0)
        return false;

    (void)client_fd;

    std::string response;

    if (message == "!bot help")
        response = "BOT: Available commands: !bot help, !bot time, !bot users, !bot joke";
    else if (message == "!bot time")
    {
        time_t now = time(0);
        char *dt = ctime(&now);
        response = "BOT: Current server time is: ";
        response += dt;

        if (!response.empty() && response[response.length() - 1] == '\n')
            response.erase(response.length() - 1);
    }
    else if (message == "!bot users")
    {
        Channel &chan = env->getChannel(channel_name);
        std::ostringstream oss; // we can use instead std::to_string() to convert numbers.
        oss << "BOT: There are " << chan.getClients().size() << " users in this channel";
        response = oss.str();
    }
    else if(message == "!bot joke")
    {
        const char* jokes[] = {
             "Why do programmers prefer dark mode? Because light attracts bugs!",
            "How many programmers does it take to change a light bulb? None, that's a hardware problem!",
            "Why did the programmer quit his job? Because he didn't get arrays!",
            "What's a programmer's favorite hangout place? Foo Bar!",
            "Why do Java developers wear glasses? Because they don't C#!"
        };
        int random_idx = time(0) % 5;
        response = "BOT: ";
        response += jokes[random_idx];
    }
    else
    {
        response = "BOT: Unknown command. Type '!bot help' for available commands.";
    }

    std::string bot_msg = ":BOT PRIVMSG " + channel_name + " :" + response + "\r\n";
    broadcast_to_channel(bot_msg, channel_name, env);

    std::cout << "BOT responded in " << channel_name << ": " << response << std::endl;

    return true;
}