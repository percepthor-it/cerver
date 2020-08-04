#ifndef JWT_ALG_H
#define JWT_ALG_H

/** JWT algorithm types. */
typedef enum jwt_alg {
	JWT_ALG_NONE = 0,
	JWT_ALG_HS256,
	JWT_ALG_HS384,
	JWT_ALG_HS512,
	JWT_ALG_RS256,
	JWT_ALG_RS384,
	JWT_ALG_RS512,
	JWT_ALG_ES256,
	JWT_ALG_ES384,
	JWT_ALG_ES512,
	JWT_ALG_TERM
} jwt_alg_t;

#define JWT_ALG_INVAL JWT_ALG_TERM

#endif