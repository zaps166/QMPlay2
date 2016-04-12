static const float verticesYCbCr[8][8] = {
	/* Normal */
	{
		-1.0f, -1.0f, //0. Left-bottom
		+1.0f, -1.0f, //1. Right-bottom
		-1.0f, +1.0f, //2. Left-top
		+1.0f, +1.0f, //3. Right-top
	},
	/* Horizontal flip */
	{
		+1.0f, -1.0f, //1. Right-bottom
		-1.0f, -1.0f, //0. Left-bottom
		+1.0f, +1.0f, //3. Right-top
		-1.0f, +1.0f, //2. Left-top
	},
	/* Vertical flip */
	{
		-1.0f, +1.0f, //2. Left-top
		+1.0f, +1.0f, //3. Right-top
		-1.0f, -1.0f, //0. Left-bottom
		+1.0f, -1.0f, //1. Right-bottom
	},
	/* Rotated 180 */
	{
		+1.0f, +1.0f, //3. Right-top
		-1.0f, +1.0f, //2. Left-top
		+1.0f, -1.0f, //1. Right-bottom
		-1.0f, -1.0f, //0. Left-bottom
	},

	/* Rotated 90 */
	{
		-1.0f, +1.0f, //2. Left-top
		-1.0f, -1.0f, //0. Left-bottom
		+1.0f, +1.0f, //3. Right-top
		+1.0f, -1.0f, //1. Right-bottom
	},
	/* Rotated 90 + horizontal flip */
	{
		+1.0f, +1.0f, //3. Right-top
		+1.0f, -1.0f, //1. Right-bottom
		-1.0f, +1.0f, //2. Left-top
		-1.0f, -1.0f, //0. Left-bottom
	},
	/* Rotated 90 + vertical flip */
	{
		-1.0f, -1.0f, //0. Left-bottom
		-1.0f, +1.0f, //2. Left-top
		+1.0f, -1.0f, //1. Right-bottom
		+1.0f, +1.0f, //3. Right-top
	},
	/* Rotated 270 */
	{
		+1.0f, -1.0f, //1. Right-bottom
		+1.0f, +1.0f, //3. Right-top
		-1.0f, -1.0f, //0. Left-bottom
		-1.0f, +1.0f, //2. Left-top
	},
};
static const float texCoordOSD[8] = {
	0.0f, 1.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
};
