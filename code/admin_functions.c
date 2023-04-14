#include "admin_functions.h"

void handle_admin_console(char *buffer, int sockfd, int slen, struct sockaddr_in cliaddr){

    int recv_len;

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
                sendto(sockfd, "User added successfully\n", strlen("User added successfully\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            } else {
                sendto(sockfd, "Failed to add user\n", strlen("Failed to add user\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
                }
        } 
        else if (strcmp(token, "DEL") == 0) {

            char* username = strtok(NULL, " \n");

            if (deleteUser(username)) {
                sendto(sockfd, "User deleted successfully\n", strlen("User deleted successfully\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            }
            else {
                sendto(sockfd, "Failed to delete user\n", strlen("Failed to delete user\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            }
        } 
        else if (strcmp(token, "LIST\n") == 0) {
            listUsers();
            sendto(sockfd, "User list sent\n", strlen("User list sent\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
        }
        else if (strcmp(token, "QUIT\n") == 0) {
            quitConsole();
            break;
        } 
        else if (strcmp(token, "QUIT_SERVER\n") == 0) {
            quitServer(sockfd);
        }
        else {
            sendto(sockfd, "Invalid command\n", strlen("Invalid command\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
        }
    }
}

bool admin_authentication(char *buffer, int sockfd, int slen, struct sockaddr_in cliaddr){

    int recv_len;

    while (1) {

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
            printf("Authentication successful.\n");
            return true;
        } 
        else {
            printf("Authentication failed.\n");
        }
    }
}

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
void listUsers() {

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
        printf("Username: %s\n", token);
        token = strtok(NULL, ";");
        printf("Password: %s\n", token);
        token = strtok(NULL, ";\n");
        printf("User type: %s\n\n", token);
    }

    fclose(fp);
}

// Function to handle QUIT command
void quitConsole() {
    printf("Admin console session ended.\n");
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
