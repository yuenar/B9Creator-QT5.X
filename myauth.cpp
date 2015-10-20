#include "myauth.h"
#include<openssl/rsa.h>
#include<openssl/pem.h>
#include<openssl/err.h>

myauth::myauth(void)
{
	

}
QString myauth::decodekey(QString str)
{
	char *p_en;
     RSA *p_rsa;
	// QByteArray dd = str.toLatin1();
	 char* data ;//= dd.data();
	 char *public_cert = "-----BEGIN PUBLIC KEY-----\n\
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDC05BfGiUjFU99xFPBYtuRqMbu\n\
AzlyjIq2TZRIW/gvifJhsivjNlp6tqNM9Oz//dC13/2+/f2SLbYXfLlQJFmgE0X5\n\
RK930WgQjg1X21t0MJyKzbsqTim/PNW9JfwGAODVvDq9XKEmvDf3Nm3sli5NSjYo\n\
DMkSSzx7vAVkTxkRJQIDAQAB\n\
-----END PUBLIC KEY-----\n";
	 BIO *key;
	 key = BIO_new_mem_buf(public_cert, -1);
	// key = BIO_new(BIO_s_file());
	 //QString keyPath = QCoreApplication::applicationDirPath()+"/public.rsa";
	 //BIO_read_filename(key, keyPath.toLatin1().data()); 
     int flen,rsa_len;
	 if((p_rsa=PEM_read_bio_RSA_PUBKEY(key,NULL,NULL,NULL))==NULL){
     //if((p_rsa=PEM_read_RSAPublicKey(file,NULL,NULL,NULL))==NULL){   换成这句死活通不过，无论是否将公钥分离源文件
        BIO_free(key);
        return NULL;
     }  
	 BIO_free(key);
     flen=strlen(data);
     rsa_len=RSA_size(p_rsa);
     p_en=(char *)malloc(rsa_len+1);
     memset(p_en,0,rsa_len+1);
     if(RSA_public_encrypt(rsa_len,(unsigned char *)data,(unsigned char*)p_en,p_rsa,RSA_NO_PADDING)<0){
         return NULL;
    }
     RSA_free(p_rsa);
	 QString result;   
	 result.append(QByteArray::fromRawData(p_en,rsa_len).toBase64());
	 free(p_en);
}

myauth::~myauth(void)
{
}
