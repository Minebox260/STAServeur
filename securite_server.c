#include <stdio.h>
#include <stdlib.h>
#include <openssl/rand.h>
#include <openssl/des.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>


 // fonction pour decrypter les messages :
unsigned char encrypte_client(const unsigned char *plaintext, int plaintext_len) {
  RSA *public_key = NULL;
  FILE *fp =fopen("cle_public_voiture.key","rb");
  public_key = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
  fclose(fp);
  unsigned char *ciphertext = NULL;
  RSA_public_encrypt(plaintext_len,plaintext,ciphertext, public_key,RSA_PKCS1_PADDING);
  return *ciphertext;
}
//fonction pour decrypter les messages
unsigned char decrypte_client(const unsigned char *ciphertext, int ciphertext_len) {
  //chargement de la clé
  RSA *private_key = NULL;
  FILE *fp =fopen("cle_privee_serveur.key","rb");
  private_key = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
  fclose(fp);
  //decryptage
  unsigned char *plaintext;
  RSA_private_decrypt(ciphertext_len,ciphertext,plaintext,private_key ,RSA_PKCS1_PADDING);
  return *plaintext;
}