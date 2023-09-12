# FILE          : loggingService.py
# PROJECT       : A3 SERVICES AND LOGGING
# programmer    : Yujung Park
# FIRST VERSION : 2023-02-20
# DESCRIPTION   : This is Logging Service. The logging service supports multiple clients concurrently. 
#                 The service accepts registered client devices and receives logging message from clients and 
#                 Logging information is stored in a single, plain text log file. 
#                 The logging service provides abuse prevention service.
# REFERENCES    : https://docs.python.org/3/howto/sockets.html
#                 https://www.digitalocean.com/community/tutorials/python-socket-programming-server-client
#                 https://superfastpython.com/thread-semaphore/
#                 http://net-informations.com/python/net/thread.htm
#                 https://www.imperva.com/learn/application-security/rate-limiting/

import sys
import socket
import threading
import datetime

# get the local host
# server_address = "10.169.92.213"
# localPort   = 20001
server_address = sys.argv[1]
localPort   = int(sys.argv[2])
bufferSize  = 1024
threadLimit = threading.Semaphore(3)

clients = []
users = []
listedUsers = ['100', '200', '300', '400', '500', '600', '700', '800', '900']
blockedClient = []
logDict = {}


# change prints to all log files later
def listenMessage(client, user):
    client_log = []
    abusecounter = 1

    while True:
        try:
            message = client.recv(1024)
            # if testing client message is quit then close the client,
            if message.decode('ascii').startswith('quit'):
                client.close()
                continue
            if not message:
                client.close()
                continue
            else:
                # check the message exist
                # if the same message is > 5, block client
                fileName = "loggingService.log"
                if (logRateLimiter(message)) != True:
                    #     print(message).decode('ascii')
                    fileLogger(message.decode('ascii'), fileName)
                    client.send('logging is completed'.encode('ascii'))
                else:
                    # receive exact same log
                    client.send('Same log exist. Service does not log for this request. '.encode('ascii'))
                    abusecounter += 1
                    if abusecounter >= 10:
                        blockClient(client, user)
                        abusecounter = 1
                        break

        except:
            index = clients.index(client)
            clients.remove(client)
            client.close()
            user = users[index]
            users.remove(user)
            break

def startService():
    # listen and accept client request 
    while True:
        client, address = server.accept()
        user = client.recv(1024).decode('ascii')

        if not user in listedUsers:
            client.send('Service Denied-Unregistered client. Please contact the server admin. '.encode('ascii'))
            client.close()
        
        if user in blockedClient:
            client.send('Service Denied-Blocked client. Please contact the server admin. '.encode('ascii'))
            client.close()

        else:
            users.append(user)
            clients.append(client)
            client.send('Connected with the server'.encode('ascii'))
            
            # print client info for server admin
            print(f'{address} : Device-{user} is connected.')

            thread = threading.Thread(target=listenMessage, args=(client, user,))
            thread.start()


def blockClient(client, user):
    if client in clients:
        clients.remove(client)
        client.send('The device is blocked due to service abuse protection, please contact the service admin. '.encode('ascii'))
        client.close()
        users.remove(user)
        blockedClient.append(user)
        print(f'{user} is blocked due to service abuse protection')

def logRateLimiter(message):
    # timestamp & msg no key
    clog = message.split()
    timestamp = clog[0]
    mNumber = clog[4]
    key = timestamp + mNumber

    if key in logDict:
        return True

    else:
        logDict[key] = message
        return False
    
def fileLogger(message, filename):
    currentTime = datetime.datetime.now()
    device = message.split()[1]
    priority = message.split()[3]
    parsed_log = message.split()
    
    level = ''
    match priority:
        case '1':
            level ='DEBUG'
        case '2':
            level = 'INFO'
        case '3':
            level = 'WARNING'
        case '4':
            level = 'ERROR'
        case other:
            level = 'CRITICAL'

    # timestamps priority type hostname message
    server_log = "{} {} {} {}\n"
    f = open(filename, "a")
    f.write(server_log.format(currentTime, device, level, message))
    f.close()


try:
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    print('Server starts')
    server.bind((server_address, localPort))
    hostname = socket.gethostname()

    # queue up as many as 5 connect request
    server.listen(5)
except socket.error as err:
    print('ERROR: %s' %(err))

startService()