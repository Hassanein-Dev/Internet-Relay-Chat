#include "server.hpp"


std::string base64_encode(const std::string &data)
{
    static const char * base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string result;
    int val = 0;
    int valb = -6;

    for(size_t i = 0; i < data.length(); i++)
    {
        unsigned char c = data[i];
        val = (val << 8) + c;
        valb += 8;
        while(valb >= 0)
        {
            result.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -=6;
        }
    }

    if (valb > -6)
    {
        result.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    while (result.size() % 4)
        result.push_back('=');
    
    return result;
}

void handle_sendfile(int client_fd, const std::string &cmd, ServerEnv *env)
{
    Client &client = env->getClient(client_fd);

    std::vector<std::string> parts = split_string(cmd, ' ');
    if (parts.size() < 3)
    {
        send_message(client_fd, "Error: Usage: SENDFILE <target> <filename> :<base64_data>\r\n");
        return;
    }

    std::string target_nick = parts[1];
    std::string filename = parts[2];

    size_t colon_pos = cmd.find(':', cmd.find(filename));
    if (colon_pos == std::string::npos)
    {
        send_message(client_fd, "Error: No file data provided\r\n");
        return;
    }

    std::string file_data = cmd.substr(colon_pos + 1);

    // Encode file data to base64
    std::string encoded_data = base64_encode(file_data);

    // Validate filename (no path traversal)
    if (filename.find('/') != std::string::npos || filename.find("..") != std::string::npos)
    {
        send_message(client_fd, "Error: Invalid filename\r\n");
        return;
    }

    int target_fd = -1;
    for(std::map<int, Client>::iterator it = env->getClients().begin();
        it != env->getClients().end(); ++it)
    {
        if(it->second.getNickname() == target_nick)
        {
            target_fd = it->first;
            break;
        }
    }

    if(target_fd == -1)
    {
        send_message(client_fd, "Error: Target user not found\r\n");
        return;
    }

    std::string file_msg = ":FILE!transfer@server PRIVMSG " + target_nick + 
                          " :File transfer from " + client.getNickname() + 
                          ": " + filename + " (" +
                          int_to_string(encoded_data.length()) + " bytes base64)\r\n";
    send_message(target_fd, file_msg);
    
    std::string data_msg = ":FILE!transfer@server NOTICE " + target_nick +
                          " :FILEDATA " + filename + " :" + encoded_data + "\r\n";
    send_message(target_fd, data_msg);

    send_message(client_fd, ":server NOTICE " + client.getNickname() + 
                " :File '" + filename + "' sent to " + target_nick + "\r\n");

    std::cout << "FILE '" << client.getNickname() << " sent '" << filename
              << "' to " << target_nick << " (" << encoded_data.length() << " bytes base64)" << std::endl;
}

void handle_getfile(int client_fd, const std::string &cmd, ServerEnv *env)
{
    Client &client = env->getClient(client_fd);

    std::vector<std::string> parts = split_string(cmd, ' ');
    if(parts.size() < 2)
    {
        send_message(client_fd, "Error: Usage: GETFILE <filename>\r\n");
        return;
    }

    std::string filename = parts[1];

    if(filename.find('/') != std::string::npos || filename.find("..") != std::string::npos)
    {
        send_message(client_fd, "Error: Invalid filename\r\n");
        return;
    }

    std::ifstream file(filename.c_str(), std::ios::binary);
    if(!file.is_open())
    {
        send_message(client_fd, "Error: File not found or cannot be read\r\n");
        return;
    }

    std::string content;
    char buffer[4096];
    while(file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
        content.append(buffer, file.gcount());

    file.close();

    if(content.length() > 10240)
    {
        send_message(client_fd, "Error: file too large (max 10KB)\r\n");
        return;
    }

    std::string encoded = base64_encode(content);

    std::string info_msg = ":server NOTICE " + client.getNickname() + 
                           " :Sending file '" + filename + "' (" +
                           int_to_string(content.length()) + " bytes)\r\n";
    send_message(client_fd, info_msg);

    //send data in chunks
    size_t chunk_size = 400;
    for(size_t i = 0; i < encoded.length(); i += chunk_size)
    {
        std::string chunk = encoded.substr(i, chunk_size);
        std::string chunk_msg = ":server NOTICE " + client.getNickname() +
                                " :FILEDATA " + filename + " :" + chunk + "\r\n";
        send_message(client_fd, chunk_msg);
    }

    send_message(client_fd, ":server NOTICE " + client.getNickname() + 
                " :File transfer complete\r\n");

    std::cout << "FILE: Sent '" << filename << "' to " << client.getNickname()
              << " (" << content.length() << " bytes)" << std::endl;
}

std::string int_to_string(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}