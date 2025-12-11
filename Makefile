NAME    := ircserv
C++     := c++
C++FLAGS:= -Wall -Wextra -Werror -std=c++98

OBJDIR  := obj
SRCS    := main.cpp \
       server_setup.cpp \
       server_loop.cpp \
       client_handler.cpp \
       auth_commands.cpp \
       utils.cpp \
       cmd_join.cpp \
       cmd_privmsg.cpp \
       cmd_operator.cpp \
       cmd_who.cpp \
       cmd_whois.cpp \
       bot.cpp \
       file_transfer.cpp
OBJS    := $(SRCS:%.cpp=$(OBJDIR)/%.o)

GREEN := \033[1;32m
RED   := \033[1;31m
RST   := \033[0m

all: $(NAME)

$(NAME): $(OBJS)
	@$(C++) $(C++FLAGS) -o $@ $^ > /dev/null 2>&1 || (printf '$(RED)Compilation failed — remove quiet to see errors$(RST)\n' && false)
	@printf '$(GREEN)%s compiled successfully$(RST)\n' $(NAME)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(OBJDIR)
	@$(C++) $(C++FLAGS) -c $< -o $@ > /dev/null 2>&1

clean:
	@rm -rf $(OBJDIR)
	@printf '$(RED)clean done$(RST)\n'

fclean: clean
	@rm -f $(NAME)
	@printf '$(RED)fclean done$(RST)\n'

re: fclean all

.PHONY: all clean fclean re