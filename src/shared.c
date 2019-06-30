/**
 * Author......: See docs/credits.txt
 * License.....: MIT
 */

#include "common.h"
#include "types.h"
#include "convert.h"
#include "shared.h"
#include "memory.h"
#include <errno.h>

#if defined (__CYGWIN__)
#include <sys/cygwin.h>
#endif

static const char *PA_000 = "OK";
static const char *PA_001 = "Ignored due to comment";
static const char *PA_002 = "Ignored due to zero length";
static const char *PA_003 = "Line-length exception";
static const char *PA_004 = "Hash-length exception";
static const char *PA_005 = "Hash-value exception";
static const char *PA_006 = "Salt-length exception";
static const char *PA_007 = "Salt-value exception";
static const char *PA_008 = "Salt-iteration count exception";
static const char *PA_009 = "Separator unmatched";
static const char *PA_010 = "Signature unmatched";
static const char *PA_011 = "Invalid hccapx file size";
static const char *PA_012 = "Invalid hccapx eapol size";
static const char *PA_013 = "Invalid psafe2 filesize";
static const char *PA_014 = "Invalid psafe3 filesize";
static const char *PA_015 = "Invalid truecrypt filesize";
static const char *PA_016 = "Invalid veracrypt filesize";
static const char *PA_017 = "Invalid SIP directive, only MD5 is supported";
static const char *PA_018 = "Hash-file exception";
static const char *PA_019 = "Hash-encoding exception";
static const char *PA_020 = "Salt-encoding exception";
static const char *PA_021 = "Invalid LUKS filesize";
static const char *PA_022 = "Invalid LUKS identifier";
static const char *PA_023 = "Invalid LUKS version";
static const char *PA_024 = "Invalid or unsupported LUKS cipher type";
static const char *PA_025 = "Invalid or unsupported LUKS cipher mode";
static const char *PA_026 = "Invalid or unsupported LUKS hash type";
static const char *PA_027 = "Invalid LUKS key size";
static const char *PA_028 = "Disabled LUKS key detected";
static const char *PA_029 = "Invalid LUKS key AF stripes count";
static const char *PA_030 = "Invalid combination of LUKS hash type and cipher type";
static const char *PA_031 = "Invalid hccapx signature";
static const char *PA_032 = "Invalid hccapx version";
static const char *PA_033 = "Invalid hccapx message pair";
static const char *PA_034 = "Token encoding exception";
static const char *PA_035 = "Token length exception";
static const char *PA_036 = "Insufficient entropy exception";
static const char *PA_037 = "Hash contains unsupported compression type for current mode";
static const char *PA_255 = "Unknown error";

static const char *OPTI_STR_OPTIMIZED_KERNEL     = "Optimized-Kernel";
static const char *OPTI_STR_ZERO_BYTE            = "Zero-Byte";
static const char *OPTI_STR_PRECOMPUTE_INIT      = "Precompute-Init";
static const char *OPTI_STR_MEET_IN_MIDDLE       = "Meet-In-The-Middle";
static const char *OPTI_STR_EARLY_SKIP           = "Early-Skip";
static const char *OPTI_STR_NOT_SALTED           = "Not-Salted";
static const char *OPTI_STR_NOT_ITERATED         = "Not-Iterated";
static const char *OPTI_STR_PREPENDED_SALT       = "Prepended-Salt";
static const char *OPTI_STR_APPENDED_SALT        = "Appended-Salt";
static const char *OPTI_STR_SINGLE_HASH          = "Single-Hash";
static const char *OPTI_STR_SINGLE_SALT          = "Single-Salt";
static const char *OPTI_STR_BRUTE_FORCE          = "Brute-Force";
static const char *OPTI_STR_RAW_HASH             = "Raw-Hash";
static const char *OPTI_STR_SLOW_HASH_SIMD_INIT  = "Slow-Hash-SIMD-INIT";
static const char *OPTI_STR_SLOW_HASH_SIMD_LOOP  = "Slow-Hash-SIMD-LOOP";
static const char *OPTI_STR_SLOW_HASH_SIMD_COMP  = "Slow-Hash-SIMD-COMP";
static const char *OPTI_STR_USES_BITS_8          = "Uses-8-Bit";
static const char *OPTI_STR_USES_BITS_16         = "Uses-16-Bit";
static const char *OPTI_STR_USES_BITS_32         = "Uses-32-Bit";
static const char *OPTI_STR_USES_BITS_64         = "Uses-64-Bit";

static const char *HASH_CATEGORY_UNDEFINED_STR              = "Undefined";
static const char *HASH_CATEGORY_RAW_HASH_STR               = "Raw Hash";
static const char *HASH_CATEGORY_RAW_HASH_SALTED_STR        = "Raw Hash, Salted and/or Iterated";
static const char *HASH_CATEGORY_RAW_HASH_AUTHENTICATED_STR = "Raw Hash, Authenticated";
static const char *HASH_CATEGORY_RAW_CIPHER_KPA_STR         = "Raw Cipher, Known-Plaintext attack";
static const char *HASH_CATEGORY_GENERIC_KDF_STR            = "Generic KDF";
static const char *HASH_CATEGORY_NETWORK_PROTOCOL_STR       = "Network Protocols";
static const char *HASH_CATEGORY_FORUM_SOFTWARE_STR         = "Forums, CMS, E-Commerce, Frameworks";
static const char *HASH_CATEGORY_DATABASE_SERVER_STR        = "Database Server";
static const char *HASH_CATEGORY_NETWORK_SERVER_STR         = "FTP, HTTP, SMTP, LDAP Server";
static const char *HASH_CATEGORY_RAW_CHECKSUM_STR           = "Raw Checksum";
static const char *HASH_CATEGORY_OS_STR                     = "Operating System";
static const char *HASH_CATEGORY_EAS_STR                    = "Enterprise Application Software (EAS)";
static const char *HASH_CATEGORY_ARCHIVE_STR                = "Archives";
static const char *HASH_CATEGORY_FDE_STR                    = "Full-Disk Encryption (FDE)";
static const char *HASH_CATEGORY_DOCUMENTS_STR              = "Documents";
static const char *HASH_CATEGORY_PASSWORD_MANAGER_STR       = "Password Managers";
static const char *HASH_CATEGORY_OTP_STR                    = "One-Time Passwords";
static const char *HASH_CATEGORY_PLAIN_STR                  = "Plaintext";

static inline int get_msb32 (const u32 v)
{
  int i;

  for (i = 32; i > 0; i--) if ((v >> (i - 1)) & 1) break;

  return i;
}

static inline int get_msb64 (const u64 v)
{
  int i;

  for (i = 64; i > 0; i--) if ((v >> (i - 1)) & 1) break;

  return i;
}

bool overflow_check_u32_add (const u32 a, const u32 b)
{
  const int a_msb = get_msb32 (a);
  const int b_msb = get_msb32 (b);

  return ((a_msb < 32) && (b_msb < 32));
}

bool overflow_check_u32_mul (const u32 a, const u32 b)
{
  const int a_msb = get_msb32 (a);
  const int b_msb = get_msb32 (b);

  return ((a_msb + b_msb) < 32);
}

bool overflow_check_u64_add (const u64 a, const u64 b)
{
  const int a_msb = get_msb64 (a);
  const int b_msb = get_msb64 (b);

  return ((a_msb < 64) && (b_msb < 64));
}

bool overflow_check_u64_mul (const u64 a, const u64 b)
{
  const int a_msb = get_msb64 (a);
  const int b_msb = get_msb64 (b);

  return ((a_msb + b_msb) < 64);
}

bool is_power_of_2 (const u32 v)
{
  return (v && !(v & (v - 1)));
}

u32 mydivc32 (const u32 dividend, const u32 divisor)
{
  u32 quotient = dividend / divisor;

  if (dividend % divisor) quotient++;

  return quotient;
}

u64 mydivc64 (const u64 dividend, const u64 divisor)
{
  u64 quotient = dividend / divisor;

  if (dividend % divisor) quotient++;

  return quotient;
}

char *filename_from_filepath (char *filepath)
{
  char *ptr = NULL;

  if ((ptr = strrchr (filepath, '/')) != NULL)
  {
    ptr++;
  }
  else if ((ptr = strrchr (filepath, '\\')) != NULL)
  {
    ptr++;
  }
  else
  {
    ptr = filepath;
  }

  return ptr;
}

void naive_replace (char *s, const char key_char, const char replace_char)
{
  const size_t len = strlen (s);

  for (size_t in = 0; in < len; in++)
  {
    const char c = s[in];

    if (c == key_char)
    {
      s[in] = replace_char;
    }
  }
}

void naive_escape (char *s, size_t s_max, const char key_char, const char escape_char)
{
  char s_escaped[1024] = { 0 };

  size_t s_escaped_max = sizeof (s_escaped);

  const size_t len = strlen (s);

  for (size_t in = 0, out = 0; in < len; in++, out++)
  {
    const char c = s[in];

    if (c == key_char)
    {
      s_escaped[out] = escape_char;

      out++;
    }

    if (out == s_escaped_max - 2) break;

    s_escaped[out] = c;
  }

  strncpy (s, s_escaped, s_max - 1);
}

int hc_asprintf (char **strp, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  int rc = vasprintf (strp, fmt, args);
  va_end (args);
  return rc;
}

#if defined (_WIN)
#define __WINDOWS__
#endif
#include "sort_r.h"
#if defined (_WIN)
#undef __WINDOWS__
#endif

void hc_qsort_r (void *base, size_t nmemb, size_t size, int (*compar) (const void *, const void *, void *), void *arg)
{
  sort_r (base, nmemb, size, compar, arg);
}

void *hc_bsearch_r (const void *key, const void *base, size_t nmemb, size_t size, int (*compar) (const void *, const void *, void *), void *arg)
{
  for (size_t l = 0, r = nmemb; r; r >>= 1)
  {
    const size_t m = r >> 1;

    const size_t c = l + m;

    const char *next = (const char *) base + (c * size);

    const int cmp = (*compar) (key, next, arg);

    if (cmp > 0)
    {
      l += m + 1;

      r--;
    }

    if (cmp == 0) return ((void *) next);
  }

  return (NULL);
}

bool hc_path_is_file (const char *path)
{
  struct stat s;

  if (stat (path, &s) == -1) return false;

  if (S_ISREG (s.st_mode)) return true;

  return false;
}

bool hc_path_is_directory (const char *path)
{
  struct stat s;

  if (stat (path, &s) == -1) return false;

  if (S_ISDIR (s.st_mode)) return true;

  return false;
}

bool hc_path_is_empty (const char *path)
{
  struct stat s;

  if (stat (path, &s) == -1) return false;

  if (s.st_size == 0) return true;

  return false;
}

bool hc_path_exist (const char *path)
{
  if (access (path, F_OK) == -1) return false;

  return true;
}

bool hc_path_read (const char *path)
{
  if (access (path, R_OK) == -1) return false;

  return true;
}

bool hc_path_write (const char *path)
{
  if (access (path, W_OK) == -1) return false;

  return true;
}

bool hc_path_create (const char *path)
{
  if (hc_path_exist (path) == true) return false;

  const int fd = creat (path, S_IRUSR | S_IWUSR);

  if (fd == -1) return false;

  close (fd);

  unlink (path);

  return true;
}

bool hc_path_has_bom (const char *path)
{
  u8 buf[8] = { 0 };

  HCFILE fp;

  if (hc_fopen (&fp, path, "rb", HCFILE_FORMAT_PLAIN) == false) return false;

  const size_t nread = hc_fread (buf, 1, sizeof (buf), &fp);

  hc_fclose (&fp);

  if (nread < 1) return false;

  /* signatures from https://en.wikipedia.org/wiki/Byte_order_mark#Byte_order_marks_by_encoding */

  // utf-8

  if ((buf[0] == 0xef)
   && (buf[1] == 0xbb)
   && (buf[2] == 0xbf)) return true;

  // utf-16

  if ((buf[0] == 0xfe)
   && (buf[1] == 0xff)) return true;

  if ((buf[0] == 0xff)
   && (buf[1] == 0xfe)) return true;

  // utf-32

  if ((buf[0] == 0x00)
   && (buf[1] == 0x00)
   && (buf[2] == 0xfe)
   && (buf[3] == 0xff)) return true;

  if ((buf[0] == 0xff)
   && (buf[1] == 0xfe)
   && (buf[2] == 0x00)
   && (buf[3] == 0x00)) return true;

  // utf-7

  if ((buf[0] == 0x2b)
   && (buf[1] == 0x2f)
   && (buf[2] == 0x76)
   && (buf[3] == 0x38)) return true;

  if ((buf[0] == 0x2b)
   && (buf[1] == 0x2f)
   && (buf[2] == 0x76)
   && (buf[3] == 0x39)) return true;

  if ((buf[0] == 0x2b)
   && (buf[1] == 0x2f)
   && (buf[2] == 0x76)
   && (buf[3] == 0x2b)) return true;

  if ((buf[0] == 0x2b)
   && (buf[1] == 0x2f)
   && (buf[2] == 0x76)
   && (buf[3] == 0x2f)) return true;

  if ((buf[0] == 0x2b)
   && (buf[1] == 0x2f)
   && (buf[2] == 0x76)
   && (buf[3] == 0x38)
   && (buf[4] == 0x2d)) return true;

  // utf-1

  if ((buf[0] == 0xf7)
   && (buf[1] == 0x64)
   && (buf[2] == 0x4c)) return true;

  // utf-ebcdic

  if ((buf[0] == 0xdd)
   && (buf[1] == 0x73)
   && (buf[2] == 0x66)
   && (buf[3] == 0x73)) return true;

  // scsu

  if ((buf[0] == 0x0e)
   && (buf[1] == 0xfe)
   && (buf[2] == 0xff)) return true;

  // bocu-1

  if ((buf[0] == 0xfb)
   && (buf[1] == 0xee)
   && (buf[2] == 0x28)) return true;

  // gb-18030

  if ((buf[0] == 0x84)
   && (buf[1] == 0x31)
   && (buf[2] == 0x95)
   && (buf[3] == 0x33)) return true;

  return false;
}

bool hc_string_is_digit (const char *s)
{
  if (s == NULL) return false;

  const size_t len = strlen (s);

  if (len == 0) return false;

  for (size_t i = 0; i < len; i++)
  {
    const int c = (const int) s[i];

    if (isdigit (c) == 0) return false;
  }

  return true;
}

void setup_environment_variables (const folder_config_t *folder_config)
{
  char *compute = getenv ("COMPUTE");

  if (compute)
  {
    char *display;

    hc_asprintf (&display, "DISPLAY=%s", compute);

    putenv (display);

    free (display);
  }
  else
  {
    if (getenv ("DISPLAY") == NULL)
      putenv ((char *) "DISPLAY=:0");
  }

  /*
  if (getenv ("OCL_CODE_CACHE_ENABLE") == NULL)
    putenv ((char *) "OCL_CODE_CACHE_ENABLE=0");

  if (getenv ("CUDA_CACHE_DISABLE") == NULL)
    putenv ((char *) "CUDA_CACHE_DISABLE=1");

  if (getenv ("POCL_KERNEL_CACHE") == NULL)
    putenv ((char *) "POCL_KERNEL_CACHE=0");
  */

  if (getenv ("TMPDIR") == NULL)
  {
    char *tmpdir = NULL;

    hc_asprintf (&tmpdir, "TMPDIR=%s", folder_config->profile_dir);

    putenv (tmpdir);

    // we can't free tmpdir at this point!
  }

  if (getenv ("CL_CONFIG_USE_VECTORIZER") == NULL)
    putenv ((char *) "CL_CONFIG_USE_VECTORIZER=False");

  #if defined (__CYGWIN__)
  cygwin_internal (CW_SYNC_WINENV);
  #endif
}

void setup_umask ()
{
  umask (077);
}

void setup_seeding (const bool rp_gen_seed_chgd, const u32 rp_gen_seed)
{
  if (rp_gen_seed_chgd == true)
  {
    srand (rp_gen_seed);
  }
  else
  {
    const time_t ts = time (NULL); // don't tell me that this is an insecure seed

    srand ((unsigned int) ts);
  }
}

u32 get_random_num (const u32 min, const u32 max)
{
  if (min == max) return (min);

  const u32 low = max - min;

  if (low == 0) return (0);

  #if defined (_WIN)

  return (((u32) rand () % (max - min)) + min);

  #else

  return (((u32) random () % (max - min)) + min);

  #endif
}

void hc_string_trim_leading (char *s)
{
  int skip = 0;

  const int len = (int) strlen (s);

  for (int i = 0; i < len; i++)
  {
    const int c = (const int) s[i];

    if (isspace (c) == 0) break;

    skip++;
  }

  if (skip == 0) return;

  const int new_len = len - skip;

  memmove (s, s + skip, new_len);

  s[new_len] = 0;
}

void hc_string_trim_trailing (char *s)
{
  int skip = 0;

  const int len = (int) strlen (s);

  for (int i = len - 1; i >= 0; i--)
  {
    const int c = (const int) s[i];

    if (isspace (c) == 0) break;

    skip++;
  }

  if (skip == 0) return;

  const size_t new_len = len - skip;

  s[new_len] = 0;
}

#if defined (__CYGWIN__)
// workaround for zlib with cygwin build
int _wopen(const char *path, int oflag, ...)
{
  va_list ap;
  va_start (ap, oflag);
  int r = open (path, oflag, ap);
  va_end (ap);
  return r;
}
#endif

bool hc_fopen (HCFILE *fp, const char *path, char *mode, int file_format)
{
  if (!path || !mode) return false;

  int oflag = -1;

  int fmode = S_IRUSR|S_IWUSR;

  if (!strncmp (mode, "a", 1) || !strncmp (mode, "ab", 2))
  {
    oflag = O_WRONLY | O_CREAT | O_APPEND;

    #if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(_WIN32) || defined(__CYGWIN__)
    if (!strncmp (mode, "ab", 2)) oflag |= O_BINARY;
    #endif
  }
  else if (!strncmp (mode, "r", 1) || !strncmp (mode, "rb", 2))
  {
    oflag = O_RDONLY;
    fmode = -1;

    #if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(_WIN32) || defined(__CYGWIN__)
    if (!strncmp (mode, "rb", 2)) oflag |= O_BINARY;
    #endif
  }
  else if (!strncmp (mode, "w", 1) || !strncmp (mode, "wb", 2))
  {
    oflag = O_WRONLY | O_CREAT | O_TRUNC;

    #if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(_WIN32) || defined(__CYGWIN__)
    if (!strncmp (mode, "wb", 2)) oflag |= O_BINARY;
    #endif
  }
  else
  {
    // ADD more strncmp to handle more "mode"
    return false;
  }

  if (file_format == HCFILE_FORMAT_PLAIN)
  {
    unsigned char check[3] = { 0 };

    int fd_tmp = open (path, O_RDONLY);

    lseek (fd_tmp, 0, SEEK_SET);

    size_t s = read (fd_tmp, check, sizeof(check));

    if (s == 3 && (check[0] == 0x1f && check[1] == 0x8b && check[2] == 0x08)) file_format = HCFILE_FORMAT_GZIP;

    close (fd_tmp);
  }

  fp->fd = (fmode == -1) ? open (path, oflag) : open (path, oflag, fmode);

  if (fp->fd == -1) return false;

  if (file_format == HCFILE_FORMAT_PLAIN)
  {
    if (!(fp->pfp = fdopen (fp->fd, mode)))  return false;

    fp->is_gzip = false;
  }
  else // HCFILE_FORMAT_GZIP
  {
    if (!(fp->gfp = gzdopen (fp->fd, mode))) return false;

    fp->is_gzip = true;
    fp->pfp     = NULL;
  }

  fp->path = path;
  fp->mode = mode;

  return true;
}

size_t hc_fread (void *ptr, size_t size, size_t nmemb, HCFILE *fp)
{
  size_t n = -1;

  if (fp == NULL) return n;

  if (fp->is_gzip)
    n = gzfread (ptr, size, nmemb, fp->gfp);
  else
    n = fread (ptr, size, nmemb, fp->pfp);

  return n;
}

size_t hc_fwrite (void *ptr, size_t size, size_t nmemb, HCFILE *fp)
{
  size_t n = -1;

  if (fp == NULL) return n;

  if (fp->is_gzip)
    n = gzfwrite (ptr, size, nmemb, fp->gfp);
  else
    n = fwrite (ptr, size, nmemb, fp->pfp);

  if (n != nmemb) return -1;

  return n;
}

int hc_fseek (HCFILE *fp, off_t offset, int whence)
{
  int r = -1;

  if (fp == NULL) return r;

  if (fp->is_gzip)
    r = gzseek (fp->gfp, (z_off_t) offset, whence);
  else
    r = fseeko (fp->pfp, offset, whence);

  return r;
}

void hc_rewind (HCFILE *fp)
{
  if (fp == NULL) return;

  if (fp->is_gzip)
    gzrewind (fp->gfp);
  else
    rewind (fp->pfp);
}

off_t hc_ftell (HCFILE *fp)
{
  off_t n = 0;

  if (fp == NULL) return -1;

  if (fp->is_gzip)
    n = (off_t) gztell (fp->gfp);
  else
    n = ftello (fp->pfp);

  return n;
}

int hc_fputc (int c, HCFILE *fp)
{
  int r = -1;

  if (fp == NULL) return r;

  if (fp->is_gzip)
    r = gzputc (fp->gfp, c);
  else
    r = fputc (c, fp->pfp);

  return r;
}

int hc_fgetc (HCFILE *fp)
{
  int r = -1;

  if (fp == NULL) return r;

  if (fp->is_gzip)
    r = gzgetc (fp->gfp);
  else
    r = fgetc (fp->pfp);

  return r;
}

char *hc_fgets (char *buf, int len, HCFILE *fp)
{
  char *r = NULL;

  if (fp == NULL) return NULL;

  if (fp->is_gzip)
    r = gzgets (fp->gfp, buf, len);
  else
    r = fgets (buf, len, fp->pfp);

  return r;
}

int hc_vfprintf (HCFILE *fp, const char *format, va_list ap)
{
  int r = -1;

  if (fp == NULL) return r;

  if (fp->is_gzip)
    r = gzvprintf (fp->gfp, format, ap);
  else
    r = vfprintf (fp->pfp, format, ap);

  return r;
}

int hc_fprintf (HCFILE *fp, const char *format, ...)
{
  int r = -1;

  if (fp == NULL) return r;

  va_list ap;

  va_start (ap, format);

  if (fp->is_gzip)
    r = gzvprintf (fp->gfp, format, ap);
  else
    r = vfprintf (fp->pfp, format, ap);

  va_end (ap);

  return r;
}

int hc_fscanf (HCFILE *fp, const char *format, void *ptr)
{
  if (fp == NULL) return -1;

  char *buf = (char *) hcmalloc (HCBUFSIZ_TINY);

  if (!buf) return -1;

  char *b = hc_fgets (buf, HCBUFSIZ_TINY - 1, fp);

  if (!b)
  {
    hcfree (buf);

    return -1;
  }

  sscanf (b, format, (void *) ptr);

  hcfree (buf);

  return 1;
}

int hc_fileno (HCFILE *fp)
{
  if (fp == NULL) return 1;

  return fp->fd;
}

int hc_feof (HCFILE *fp)
{
  int r = -1;

  if (fp == NULL) return r;

  if (fp->is_gzip)
    r = gzeof (fp->gfp);
  else
    r = feof (fp->pfp);

  return r;
}

void hc_fflush (HCFILE *fp)
{
  if (fp == NULL) return;

  if (fp->is_gzip)
    gzflush (fp->gfp, Z_SYNC_FLUSH);
 else
    fflush (fp->pfp);
}

void hc_fclose (HCFILE *fp)
{
  if (fp == NULL) return;

  if (fp->is_gzip)
    gzclose (fp->gfp);
  else
    fclose (fp->pfp);

  close (fp->fd);

  fp->fd = -1;
  fp->is_gzip = false;

  fp->path = NULL;
  fp->mode = NULL;
}

bool hc_same_files (char *file1, char *file2)
{
  if ((file1 != NULL) && (file2 != NULL))
  {
    struct stat tmpstat_file1;
    struct stat tmpstat_file2;

    int do_check = 0;

    HCFILE fp;

    if (hc_fopen (&fp, file1, "r", HCFILE_FORMAT_PLAIN) == true)
    {
      if (fstat (hc_fileno (&fp), &tmpstat_file1))
      {
        hc_fclose (&fp);

        return false;
      }

      hc_fclose (&fp);

      do_check++;
    }

    if (hc_fopen (&fp, file2, "r", HCFILE_FORMAT_PLAIN) == true)
    {
      if (fstat (hc_fileno (&fp), &tmpstat_file2))
      {
        hc_fclose (&fp);

        return false;
      }

      hc_fclose (&fp);

      do_check++;
    }

    if (do_check == 2)
    {
      tmpstat_file1.st_mode     = 0;
      tmpstat_file1.st_nlink    = 0;
      tmpstat_file1.st_uid      = 0;
      tmpstat_file1.st_gid      = 0;
      tmpstat_file1.st_rdev     = 0;
      tmpstat_file1.st_atime    = 0;

      #if defined (STAT_NANOSECONDS_ACCESS_TIME)
      tmpstat_file1.STAT_NANOSECONDS_ACCESS_TIME = 0;
      #endif

      #if defined (_POSIX)
      tmpstat_file1.st_blksize  = 0;
      tmpstat_file1.st_blocks   = 0;
      #endif

      tmpstat_file2.st_mode     = 0;
      tmpstat_file2.st_nlink    = 0;
      tmpstat_file2.st_uid      = 0;
      tmpstat_file2.st_gid      = 0;
      tmpstat_file2.st_rdev     = 0;
      tmpstat_file2.st_atime    = 0;

      #if defined (STAT_NANOSECONDS_ACCESS_TIME)
      tmpstat_file2.STAT_NANOSECONDS_ACCESS_TIME = 0;
      #endif

      #if defined (_POSIX)
      tmpstat_file2.st_blksize  = 0;
      tmpstat_file2.st_blocks   = 0;
      #endif

      if (memcmp (&tmpstat_file1, &tmpstat_file2, sizeof (struct stat)) == 0)
      {
        return true;
      }
    }
  }

  return false;
}

u32 hc_strtoul (const char *nptr, char **endptr, int base)
{
  return (u32) strtoul (nptr, endptr, base);
}

u64 hc_strtoull (const char *nptr, char **endptr, int base)
{
  return (u64) strtoull (nptr, endptr, base);
}

u32 power_of_two_ceil_32 (const u32 v)
{
  u32 r = v;

  r--;

  r |= r >> 1;
  r |= r >> 2;
  r |= r >> 4;
  r |= r >> 8;
  r |= r >> 16;

  r++;

  return r;
}

u32 power_of_two_floor_32 (const u32 v)
{
  u32 r = power_of_two_ceil_32 (v);

  if (r > v)
  {
    r >>= 1;
  }

  return r;
}

u32 round_up_multiple_32 (const u32 v, const u32 m)
{
  if (m == 0) return v;

  const u32 r = v % m;

  if (r == 0) return v;

  return v + m - r;
}

u64 round_up_multiple_64 (const u64 v, const u64 m)
{
  if (m == 0) return v;

  const u64 r = v % m;

  if (r == 0) return v;

  return v + m - r;
}

// difference to original strncat is no returncode and u8* instead of char*

void hc_strncat (u8 *dst, const u8 *src, const size_t n)
{
  const size_t dst_len = strlen ((char *) dst);

  const u8 *src_ptr = src;

  u8 *dst_ptr = dst + dst_len;

  for (size_t i = 0; i < n && *src_ptr != 0; i++)
  {
    *dst_ptr++ = *src_ptr++;
  }

  *dst_ptr = 0;
}

int count_char (const u8 *buf, const int len, const u8 c)
{
  int r = 0;

  for (int i = 0; i < len; i++)
  {
    if (buf[i] == c) r++;
  }

  return r;
}

float get_entropy (const u8 *buf, const int len)
{
  float entropy = 0.0;

  for (int c = 0; c < 256; c++)
  {
    const int r = count_char (buf, len, (const u8) c);

    if (r == 0) continue;

    float w = (float) r / len;

    entropy += -w * log2 (w);
  }

  return entropy;
}

int select_read_timeout (int sockfd, const int sec)
{
  struct timeval tv;

  tv.tv_sec  = sec;
  tv.tv_usec = 0;

  fd_set fds;

  FD_ZERO (&fds);
  FD_SET (sockfd, &fds);

  return select (sockfd + 1, &fds, NULL, NULL, &tv);
}

int select_write_timeout (int sockfd, const int sec)
{
  struct timeval tv;

  tv.tv_sec  = sec;
  tv.tv_usec = 0;

  fd_set fds;

  FD_ZERO (&fds);
  FD_SET (sockfd, &fds);

  return select (sockfd + 1, NULL, &fds, NULL, &tv);
}

#if defined (_WIN)

int select_read_timeout_console (const int sec)
{
  const HANDLE hStdIn = GetStdHandle (STD_INPUT_HANDLE);

  const DWORD rc = WaitForSingleObject (hStdIn, sec * 1000);

  if (rc == WAIT_OBJECT_0)
  {
    DWORD dwRead;

    INPUT_RECORD inRecords;

    inRecords.EventType = 0;

    PeekConsoleInput (hStdIn, &inRecords, 1, &dwRead);

    if (inRecords.EventType == 0)
    {
      // those are good ones

      return 1;
    }
    else
    {
      // but we don't want that stuff like windows focus etc. in our stream

      ReadConsoleInput (hStdIn, &inRecords, 1, &dwRead);
    }

    return select_read_timeout_console (sec);
  }
  else if (rc == WAIT_TIMEOUT)
  {
    return 0;
  }

  return -1;
}

#else

int select_read_timeout_console (const int sec)
{
  return select_read_timeout (fileno (stdin), sec);
}

#endif

const char *strhashcategory (const u32 hash_category)
{
  switch (hash_category)
  {
    case HASH_CATEGORY_UNDEFINED:               return HASH_CATEGORY_UNDEFINED_STR;
    case HASH_CATEGORY_RAW_HASH:                return HASH_CATEGORY_RAW_HASH_STR;
    case HASH_CATEGORY_RAW_HASH_SALTED:         return HASH_CATEGORY_RAW_HASH_SALTED_STR;
    case HASH_CATEGORY_RAW_HASH_AUTHENTICATED:  return HASH_CATEGORY_RAW_HASH_AUTHENTICATED_STR;
    case HASH_CATEGORY_RAW_CIPHER_KPA:          return HASH_CATEGORY_RAW_CIPHER_KPA_STR;
    case HASH_CATEGORY_GENERIC_KDF:             return HASH_CATEGORY_GENERIC_KDF_STR;
    case HASH_CATEGORY_NETWORK_PROTOCOL:        return HASH_CATEGORY_NETWORK_PROTOCOL_STR;
    case HASH_CATEGORY_FORUM_SOFTWARE:          return HASH_CATEGORY_FORUM_SOFTWARE_STR;
    case HASH_CATEGORY_DATABASE_SERVER:         return HASH_CATEGORY_DATABASE_SERVER_STR;
    case HASH_CATEGORY_NETWORK_SERVER:          return HASH_CATEGORY_NETWORK_SERVER_STR;
    case HASH_CATEGORY_RAW_CHECKSUM:            return HASH_CATEGORY_RAW_CHECKSUM_STR;
    case HASH_CATEGORY_OS:                      return HASH_CATEGORY_OS_STR;
    case HASH_CATEGORY_EAS:                     return HASH_CATEGORY_EAS_STR;
    case HASH_CATEGORY_ARCHIVE:                 return HASH_CATEGORY_ARCHIVE_STR;
    case HASH_CATEGORY_FDE:                     return HASH_CATEGORY_FDE_STR;
    case HASH_CATEGORY_DOCUMENTS:               return HASH_CATEGORY_DOCUMENTS_STR;
    case HASH_CATEGORY_PASSWORD_MANAGER:        return HASH_CATEGORY_PASSWORD_MANAGER_STR;
    case HASH_CATEGORY_OTP:                     return HASH_CATEGORY_OTP_STR;
    case HASH_CATEGORY_PLAIN:                   return HASH_CATEGORY_PLAIN_STR;
  }

  return NULL;
}

const char *stroptitype (const u32 opti_type)
{
  switch (opti_type)
  {
    case OPTI_TYPE_OPTIMIZED_KERNEL:    return OPTI_STR_OPTIMIZED_KERNEL;
    case OPTI_TYPE_ZERO_BYTE:           return OPTI_STR_ZERO_BYTE;
    case OPTI_TYPE_PRECOMPUTE_INIT:     return OPTI_STR_PRECOMPUTE_INIT;
    case OPTI_TYPE_MEET_IN_MIDDLE:      return OPTI_STR_MEET_IN_MIDDLE;
    case OPTI_TYPE_EARLY_SKIP:          return OPTI_STR_EARLY_SKIP;
    case OPTI_TYPE_NOT_SALTED:          return OPTI_STR_NOT_SALTED;
    case OPTI_TYPE_NOT_ITERATED:        return OPTI_STR_NOT_ITERATED;
    case OPTI_TYPE_PREPENDED_SALT:      return OPTI_STR_PREPENDED_SALT;
    case OPTI_TYPE_APPENDED_SALT:       return OPTI_STR_APPENDED_SALT;
    case OPTI_TYPE_SINGLE_HASH:         return OPTI_STR_SINGLE_HASH;
    case OPTI_TYPE_SINGLE_SALT:         return OPTI_STR_SINGLE_SALT;
    case OPTI_TYPE_BRUTE_FORCE:         return OPTI_STR_BRUTE_FORCE;
    case OPTI_TYPE_RAW_HASH:            return OPTI_STR_RAW_HASH;
    case OPTI_TYPE_SLOW_HASH_SIMD_INIT: return OPTI_STR_SLOW_HASH_SIMD_INIT;
    case OPTI_TYPE_SLOW_HASH_SIMD_LOOP: return OPTI_STR_SLOW_HASH_SIMD_LOOP;
    case OPTI_TYPE_SLOW_HASH_SIMD_COMP: return OPTI_STR_SLOW_HASH_SIMD_COMP;
    case OPTI_TYPE_USES_BITS_8:         return OPTI_STR_USES_BITS_8;
    case OPTI_TYPE_USES_BITS_16:        return OPTI_STR_USES_BITS_16;
    case OPTI_TYPE_USES_BITS_32:        return OPTI_STR_USES_BITS_32;
    case OPTI_TYPE_USES_BITS_64:        return OPTI_STR_USES_BITS_64;
  }

  return NULL;
}

const char *strparser (const u32 parser_status)
{
  switch (parser_status)
  {
    case PARSER_OK:                   return PA_000;
    case PARSER_COMMENT:              return PA_001;
    case PARSER_GLOBAL_ZERO:          return PA_002;
    case PARSER_GLOBAL_LENGTH:        return PA_003;
    case PARSER_HASH_LENGTH:          return PA_004;
    case PARSER_HASH_VALUE:           return PA_005;
    case PARSER_SALT_LENGTH:          return PA_006;
    case PARSER_SALT_VALUE:           return PA_007;
    case PARSER_SALT_ITERATION:       return PA_008;
    case PARSER_SEPARATOR_UNMATCHED:  return PA_009;
    case PARSER_SIGNATURE_UNMATCHED:  return PA_010;
    case PARSER_HCCAPX_FILE_SIZE:     return PA_011;
    case PARSER_HCCAPX_EAPOL_LEN:     return PA_012;
    case PARSER_PSAFE2_FILE_SIZE:     return PA_013;
    case PARSER_PSAFE3_FILE_SIZE:     return PA_014;
    case PARSER_TC_FILE_SIZE:         return PA_015;
    case PARSER_VC_FILE_SIZE:         return PA_016;
    case PARSER_SIP_AUTH_DIRECTIVE:   return PA_017;
    case PARSER_HASH_FILE:            return PA_018;
    case PARSER_HASH_ENCODING:        return PA_019;
    case PARSER_SALT_ENCODING:        return PA_020;
    case PARSER_LUKS_FILE_SIZE:       return PA_021;
    case PARSER_LUKS_MAGIC:           return PA_022;
    case PARSER_LUKS_VERSION:         return PA_023;
    case PARSER_LUKS_CIPHER_TYPE:     return PA_024;
    case PARSER_LUKS_CIPHER_MODE:     return PA_025;
    case PARSER_LUKS_HASH_TYPE:       return PA_026;
    case PARSER_LUKS_KEY_SIZE:        return PA_027;
    case PARSER_LUKS_KEY_DISABLED:    return PA_028;
    case PARSER_LUKS_KEY_STRIPES:     return PA_029;
    case PARSER_LUKS_HASH_CIPHER:     return PA_030;
    case PARSER_HCCAPX_SIGNATURE:     return PA_031;
    case PARSER_HCCAPX_VERSION:       return PA_032;
    case PARSER_HCCAPX_MESSAGE_PAIR:  return PA_033;
    case PARSER_TOKEN_ENCODING:       return PA_034;
    case PARSER_TOKEN_LENGTH:         return PA_035;
    case PARSER_INSUFFICIENT_ENTROPY: return PA_036;
    case PARSER_PKZIP_CT_UNMATCHED:   return PA_037;
  }

  return PA_255;
}

static int rounds_count_length (const char *input_buf, const int input_len)
{
  if (input_len >= 9) // 9 is minimum because of "rounds=X$"
  {
    static const char *rounds = "rounds=";

    if (memcmp (input_buf, rounds, 7) == 0)
    {
      char *next_pos = strchr (input_buf + 8, '$');

      if (next_pos == NULL) return -1;

      const int rounds_len = next_pos - input_buf;

      return rounds_len;
    }
  }

  return -1;
}

int input_tokenizer (const u8 *input_buf, const int input_len, token_t *token)
{
  int len_left = input_len;

  token->buf[0] = input_buf;

  int token_idx;

  for (token_idx = 0; token_idx < token->token_cnt - 1; token_idx++)
  {
    if (token->attr[token_idx] & TOKEN_ATTR_FIXED_LENGTH)
    {
      int len = token->len[token_idx];

      if (len_left < len) return (PARSER_TOKEN_LENGTH);

      token->buf[token_idx + 1] = token->buf[token_idx] + len;

      len_left -= len;
    }
    else
    {
      if (token->attr[token_idx] & TOKEN_ATTR_OPTIONAL_ROUNDS)
      {
        const int len = rounds_count_length ((const char *) token->buf[token_idx], len_left);

        token->opt_buf = token->buf[token_idx];

        token->opt_len = len; // we want an eventual -1 in here, it's used later for verification

        if (len > 0)
        {
          token->buf[token_idx] += len + 1; // +1 = separator

          len_left -= len + 1; // +1 = separator
        }
      }

      const u8 *next_pos = (const u8 *) strchr ((const char *) token->buf[token_idx], token->sep[token_idx]);

      if (next_pos == NULL) return (PARSER_SEPARATOR_UNMATCHED);

      const int len = next_pos - token->buf[token_idx];

      token->len[token_idx] = len;

      token->buf[token_idx + 1] = next_pos + 1; // +1 = separator

      len_left -= len + 1; // +1 = separator
    }
  }

  if (token->attr[token_idx] & TOKEN_ATTR_FIXED_LENGTH)
  {
    int len = token->len[token_idx];

    if (len_left != len) return (PARSER_TOKEN_LENGTH);
  }
  else
  {
    token->len[token_idx] = len_left;
  }

  // verify data

  for (token_idx = 0; token_idx < token->token_cnt; token_idx++)
  {
    if (token->attr[token_idx] & TOKEN_ATTR_VERIFY_SIGNATURE)
    {
      bool matched = false;

      for (int signature_idx = 0; signature_idx < token->signatures_cnt; signature_idx++)
      {
        if (memcmp (token->buf[token_idx], token->signatures_buf[signature_idx], token->len[token_idx]) == 0) matched = true;
      }

      if (matched == false) return (PARSER_SIGNATURE_UNMATCHED);
    }

    if (token->attr[token_idx] & TOKEN_ATTR_VERIFY_LENGTH)
    {
      if (token->len[token_idx] < token->len_min[token_idx]) return (PARSER_TOKEN_LENGTH);
      if (token->len[token_idx] > token->len_max[token_idx]) return (PARSER_TOKEN_LENGTH);
    }

    if (token->attr[token_idx] & TOKEN_ATTR_VERIFY_HEX)
    {
      if (is_valid_hex_string (token->buf[token_idx], token->len[token_idx]) == false) return (PARSER_TOKEN_ENCODING);
    }

    if (token->attr[token_idx] & TOKEN_ATTR_VERIFY_BASE64A)
    {
      if (is_valid_base64a_string (token->buf[token_idx], token->len[token_idx]) == false) return (PARSER_TOKEN_ENCODING);
    }

    if (token->attr[token_idx] & TOKEN_ATTR_VERIFY_BASE64B)
    {
      if (is_valid_base64b_string (token->buf[token_idx], token->len[token_idx]) == false) return (PARSER_TOKEN_ENCODING);
    }

    if (token->attr[token_idx] & TOKEN_ATTR_VERIFY_BASE64C)
    {
      if (is_valid_base64c_string (token->buf[token_idx], token->len[token_idx]) == false) return (PARSER_TOKEN_ENCODING);
    }
  }

  return PARSER_OK;
}

bool generic_salt_decode (MAYBE_UNUSED const hashconfig_t *hashconfig, const u8 *in_buf, const int in_len, u8 *out_buf, int *out_len)
{
  u32 tmp_u32[(64 * 2) + 1] = { 0 };

  u8 *tmp_u8 = (u8 *) tmp_u32;

  if (in_len > 512) return false; // 512 = 2 * 256 -- (2 * because of hex), 256 because of maximum salt length in salt_t

  int tmp_len = 0;

  if (hashconfig->opts_type & OPTS_TYPE_ST_HEX)
  {
    if (in_len < (int) (hashconfig->salt_min * 2)) return false;
    if (in_len > (int) (hashconfig->salt_max * 2)) return false;

    if (in_len & 1) return false;

    for (int i = 0, j = 0; j < in_len; i += 1, j += 2)
    {
      u8 p0 = in_buf[j + 0];
      u8 p1 = in_buf[j + 1];

      tmp_u8[i]  = hex_convert (p1) << 0;
      tmp_u8[i] |= hex_convert (p0) << 4;
    }

    tmp_len = in_len / 2;
  }
  else if (hashconfig->opts_type & OPTS_TYPE_ST_BASE64)
  {
    if (in_len < (int) (((hashconfig->salt_min * 8) / 6) + 0)) return false;
    if (in_len > (int) (((hashconfig->salt_max * 8) / 6) + 3)) return false;

    tmp_len = base64_decode (base64_to_int, (const u8 *) in_buf, in_len, tmp_u8);
  }
  else
  {
    if (in_len < (int) hashconfig->salt_min) return false;
    if (in_len > (int) hashconfig->salt_max) return false;

    memcpy (tmp_u8, in_buf, in_len);

    tmp_len = in_len;
  }

  if (hashconfig->opts_type & OPTS_TYPE_ST_UTF16LE)
  {
    if (tmp_len >= 128) return false;

    for (int i = 64 - 1; i >= 1; i -= 2)
    {
      const u32 v = tmp_u32[i / 2];

      tmp_u32[i - 0] = ((v >> 8) & 0x00FF0000) | ((v >> 16) & 0x000000FF);
      tmp_u32[i - 1] = ((v << 8) & 0x00FF0000) | ((v >>  0) & 0x000000FF);
    }

    tmp_len = tmp_len * 2;
  }

  if (hashconfig->opts_type & OPTS_TYPE_ST_LOWER)
  {
    lowercase (tmp_u8, tmp_len);
  }

  if (hashconfig->opts_type & OPTS_TYPE_ST_UPPER)
  {
    uppercase (tmp_u8, tmp_len);
  }

  int tmp2_len = tmp_len;

  if (hashconfig->opts_type & OPTS_TYPE_ST_ADD80)
  {
    if (tmp2_len >= 256) return false;

    tmp_u8[tmp2_len++] = 0x80;
  }

  if (hashconfig->opts_type & OPTS_TYPE_ST_ADD01)
  {
    if (tmp2_len >= 256) return false;

    tmp_u8[tmp2_len++] = 0x01;
  }

  memcpy (out_buf, tmp_u8, tmp2_len);

  *out_len = tmp_len;

  return true;
}

int generic_salt_encode (MAYBE_UNUSED const hashconfig_t *hashconfig, const u8 *in_buf, const int in_len, u8 *out_buf)
{
  u32 tmp_u32[(64 * 2) + 1] = { 0 };

  u8 *tmp_u8 = (u8 *) tmp_u32;

  memcpy (tmp_u8, in_buf, in_len);

  int tmp_len = in_len;

  if (hashconfig->opts_type & OPTS_TYPE_ST_UTF16LE)
  {
    for (int i = 0, j = 0; j < in_len; i += 1, j += 2)
    {
      const u8 p = tmp_u8[j];

      tmp_u8[i] = p;
    }

    tmp_len = tmp_len / 2;
  }

  if (hashconfig->opts_type & OPTS_TYPE_ST_HEX)
  {
    for (int i = 0, j = 0; i < in_len; i += 1, j += 2)
    {
      u8_to_hex (in_buf[i], tmp_u8 + j);
    }

    tmp_len = in_len * 2;
  }
  else if (hashconfig->opts_type & OPTS_TYPE_ST_BASE64)
  {
    tmp_len = base64_encode (int_to_base64, in_buf, in_len, tmp_u8);
  }

  memcpy (out_buf, tmp_u8, tmp_len);

  return tmp_len;
}
