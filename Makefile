# Configuration
CC		:= g++
CFLAGS		:= -std=c++11 -Wall
TARGET 		:= saallicht
LIBS		:= -leibclient -lhidapi-hidraw
SRCDIR		:= src/
OBJDIR		:= obj/

# Sources
SRCS		:= $(wildcard  $(SRCDIR)*.cpp)
OBJS		:= $(addprefix $(OBJDIR),$(notdir $(SRCS:.cpp=.o)))
OBJS		:= $(OBJS:.c=.o)

all: $(TARGET)
	@echo Compilation succesful

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(TARGET) $(LIBS)

$(OBJDIR)%.o: $(SRCDIR)%.cpp $(OBJDIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJDIR):
	mkdir -p obj

run: all
	./saallicht

clean:
	$(RM) *~
	$(RM) $(SRCDIR)*~
	$(RM) -r $(OBJDIR)
	$(RM) $(TARGET)
