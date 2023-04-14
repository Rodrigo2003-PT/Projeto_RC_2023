#include "admin_functions.h"

void handle_admin_console(int sockfd){

    int recv_len;
    char buffer[MAXLINE];
    struct sockaddr_in cliaddr;
    socklen_t slen = sizeof(cliaddr);

    while (1) {

        // Receive command from admin console
        if((recv_len = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr *) &cliaddr, (socklen_t *)&slen)) == -1) {
            erro("Erro no recvfrom");
        }
        buffer[recv_len] = '\0';

        // Parse the command
        char* token = strtok(buffer, " ");

        if (strcmp(token, "ADD_USER") == 0) {
            char* username = strtok(NULL, " ");
            char* password = strtok(NULL, " ");
            char* userType = strtok(NULL, " \n");
            if (addUser(username, password, userType)) {
                sendto(sockfd, "user added successfully\n", strlen("user added successfully\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            } else {
                sendto(sockfd, "failed to add user\n", strlen("failed to add user\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
                }
        } 
        else if (strcmp(token, "DEL") == 0) {

            char* username = strtok(NULL, " \n");

            if (deleteUser(username)) {
                sendto(sockfd, "user deleted successfully\n", strlen("user deleted successfully\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            }
            else {
                sendto(sockfd, "failed to delete user\n", strlen("failed to delete user\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            }
        } 
        else if (strcmp(token, "LIST\n") == 0) {
            listUsers(sockfd,slen,cliaddr);
        }
        else if (strcmp(token, "QUIT\n") == 0) {
            quitConsole(sockfd);
            break;
        } 
        else if (strcmp(token, "QUIT_SERVER\n") == 0) {
            quitServer(sockfd);
        }
        else {
            sendto(sockfd, "invalid command\n", strlen("invalid command\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
        }
    }
}

void admin_authentication(int sockfd){

    int recv_len;
    char buffer[MAXLINE];
    bool authenticated = false;
    struct sockaddr_in cliaddr;
    socklen_t slen = sizeof(cliaddr);

    while (!authenticated) {

        // Receive credentials from admin console
        if((recv_len = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr *) &cliaddr, (socklen_t *)&slen)) == -1) {
            erro("Erro no recvfrom");
        }
        
        // Para ignorar o restante conteúdo (anterior do buffer)
	    buffer[recv_len]='\0';

        // Parse the credentials
        char* username = strtok(buffer, " ");
        char* password = strtok(NULL, " \n");

        // Verify credentials
        if (strcmp(username, "admin") == 0 && strcmp(password, "password") == 0) {
            authenticated = true;
            sendto(sockfd, "authentication successful\n", strlen("authentication successful\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            handle_admin_console(sockfd);
        } 
        else {
            sendto(sockfd, "authentication failed: <username> <password>\n", strlen("authentication failed <username> <password>\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            printf("Authentication failed.\n");
        }
    }
}

void client_authentication(int sockfd){};

// Function to handle ADD_USER command
bool addUser(char* username, char* password, char* userType) {

    FILE *fp;
    char line[MAXLINE];
    char* token;
    bool userExists = false;
    
    // Open the file in read mode
    fp = fopen("users.txt", "r");

    // Check if the file was opened successfully
    if (fp == NULL) {
        printf("Error opening file.");
        return false;
    }

    // Loop through the file line by line
    while (fgets(line, MAXLINE, fp) != NULL) {

        // Split the line using strtok() to extract username, password, and userType
        token = strtok(line, ";");
        char* existingUsername = token;

        // Check if the username already exists in the file
        if (strcmp(username, existingUsername) == 0) {
            userExists = true;
            break;
        }
    }

    // Close the file
    fclose(fp);

    // If user already exists, return false
    if (userExists) {
        printf("User already exists.\n");
        return false;
    }

    // Open the file in append mode
    fp = fopen("users.txt", "a");

    // Check if the file was opened successfully
    if (fp == NULL) {
        printf("Error opening file.");
        return false;
    }

    // Write the new user information to the file
    fprintf(fp, "%s;%s;%s\n", username, password, userType);

    printf("Here");

    // Close the file
    fclose(fp);

    // Return true if the user was added successfully
    return true;
}

// Function to handle DEL command
bool deleteUser(char* username) {

    char tempFile[] = "temp.txt";
    char file[] = "users.txt";
    char buffer[256];

    FILE *fp = fopen(file, "r");
    FILE *temp = fopen(tempFile, "w");

    // Check if file could be opened
    if (fp == NULL || temp == NULL) {
        printf("Error opening file.\n");
        return false;
    }

    bool found = false;

    // Read through file line by line
    while (fgets(buffer, 256, fp) != NULL) {
        
        char* line = strdup(buffer); // duplicate the line
        char* name = strtok(line, ";"); // tokenize the duplicate line

        if (strcmp(name, username) == 0) {
            found = true;
        } else {
            fputs(buffer, temp);
        }

        free(line); // free the duplicated line
    }

    fclose(fp);
    fclose(temp);

    // Delete original file and rename temp file
    remove(file);
    rename(tempFile, file);

    if (found) {
        printf("User deleted successfully.\n");
        return true;
    } else {
        printf("User not found.\n");
        return false;
    }
}

// Function to handle LIST command
void listUsers(int sockfd, int slen, struct sockaddr_in cliaddr) {

    FILE* fp;
    char* token;
    char line[MAXLINE];

    fp = fopen("users.txt", "r");

    if (fp == NULL) {
        printf("Error: could not open file\n");
        return;
    }

    while (fgets(line, MAXLINE, fp) != NULL) {
        token = strtok(line, ";");
        sendto(sockfd, token, strlen(token), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
        token = strtok(NULL, ";");
        sendto(sockfd, token, strlen(token), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
        token = strtok(NULL, ";");
        sendto(sockfd, token, strlen(token), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
    }

    fclose(fp);
}

// Function to handle QUIT command
void quitConsole(int sockfd) {
    printf("Admin console session ended.\n");
    close(sockfd);
}

// Function to handle QUIT_SERVER command
void quitServer(int sockfd) {
    printf("Server shutting down.\n");
    close(sockfd);
    exit(EXIT_SUCCESS);
}

void erro(char *s) {
	perror(s);
	exit(1);
}
