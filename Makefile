CC = gcc
FLAGS = -pthread -Wall
OBJS = system_manager.c functions.c structs.h
TARGET = 5g_auth_platform
OBJS1 = backoffice_user.c functions.c structs.h
TARGET1 =  backoffice_user
OBJS2 = mobile_user.c functions.c structs.h
TARGET2 =  mobile_user

all: $(TARGET) $(TARGET1) $(TARGET2)

$(TARGET): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $(TARGET)

$(TARGET1): $(OBJS1)
	$(CC) $(FLAGS) $(OBJS1) -o $(TARGET1)

$(TARGET2): $(OBJS2)
	$(CC) $(FLAGS) $(OBJS2) -o $(TARGET2)

clean:
		$(RM) $(TARGET)
		$(RM) $(TARGET1)
		$(RM) $(TARGET2)
