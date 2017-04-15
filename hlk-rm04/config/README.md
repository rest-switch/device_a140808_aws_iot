```
1) the host information contained host.cfg file found in this directory 
   will be placed into the /etc/config/aws-iot configuration file

2) certificate *.pem files found here will be copied to /etc/aws-iot-certs
     root_ca.pem   root ca certificate
     cert.pem      device certificate
     private.pem   device private key

     see: https://docs.aws.amazon.com/iot/latest/developerguide/device-certs-create.html

configure certificate and host information in /etc/config/aws-iot
```
