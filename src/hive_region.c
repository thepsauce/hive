#include "hex.h"

int hive_region_init(HiveRegion *region, int x, int y, int w, int h)
{
	WINDOW *win;

	if ((win = newwin(h, w, y, x)) == NULL)
		return -1;
	memset(region, 0, sizeof(*region));
	region->win = win;
	return 0;
}

int hive_region_addpiece(HiveRegion *region, HivePiece *piece)
{
	/* should in theory never happen */
	if (region->numPieces == ARRLEN(region->pieces))
		return -1;
	region->pieces[region->numPieces++] = piece;
	return 0;
}

int hive_region_removepiece(HiveRegion *region, HivePiece *piece)
{
	for (size_t i = 0; i < region->numPieces; i++) {
		if (region->pieces[i] != piece)
			continue;
		region->numPieces--;
		memmove(&region->pieces[i], &region->pieces[i + 1],
			sizeof(*region->pieces) * (region->numPieces - i));
		return 0;
	}
	/* should in theory never happen */
	return -1;
}

void hive_region_clearflags(HiveRegion *region, uint64_t flags)
{
	for (size_t i = 0; i < region->numPieces; i++)
		region->pieces[i]->flags &= ~flags;
}

HivePiece *hive_region_pieceatr(HiveRegion *region, HivePiece *from, Point at)
{
	bool doReturn;

	doReturn = from == NULL;
	for (size_t i = region->numPieces; i-- != 0; ) {
		HivePiece *piece;

		piece = region->pieces[i];
		if (piece == from) {
			doReturn = true;
			continue;
		}
		if (doReturn && point_isequal(piece->position, at))
			return piece;
	}
	return NULL;
}

HivePiece *hive_region_pieceat(HiveRegion *region, HivePiece *from, Point at)
{
	bool doReturn;

	doReturn = from == NULL;
	for (size_t i = 0; i < region->numPieces; i++) {
		HivePiece *piece;

		piece = region->pieces[i];
		if (piece == from) {
			doReturn = true;
			continue;
		}
		if (doReturn && point_isequal(piece->position, at))
			return piece;
	}
	return NULL;
}

size_t hive_region_countat(HiveRegion *region, Point at)
{
	size_t cnt = 0;

	for (size_t i = 0; i < region->numPieces; i++)
		if (point_isequal(region->pieces[i]->position, at))
			cnt++;
	return cnt;
}

size_t hive_region_getsurrounding(HiveRegion *region, Point at,
		HivePiece *pieces[6])
{
	size_t num = 0;
	for (int d = 0; d < 6; d++) {
		Point p;

		p = at;
		hive_movepoint(&p, d);
		if ((pieces[d] = hive_region_pieceat(region, NULL, p)) != NULL)
			num++;
	}
	return num;
}

size_t hive_region_getsurroundingr(HiveRegion *region, Point at,
		HivePiece *pieces[6])
{
	size_t num = 0;
	for (int d = 0; d < 6; d++) {
		Point p;

		p = at;
		hive_movepoint(&p, d);
		if ((pieces[d] = hive_region_pieceatr(region, NULL, p)) != NULL)
			num++;
	}
	return num;
}

uint32_t hive_region_count(HiveRegion *region, HivePiece *origin)
{
	/* 1 for itself */
	uint32_t cnt = 0;
	HivePiece *pieces[6];

	cnt = hive_region_countat(region, origin->position);
	origin->flags |= HIVE_VISITED;
	hive_region_getsurrounding(region, origin->position, pieces);
	for (int d = 0; d < 6; d++) {
		HivePiece *const piece = pieces[d];
		if (piece == NULL || (piece->flags & HIVE_VISITED))
			continue;
		cnt += hive_region_count(region, piece);
	}
	return cnt;
}

void hive_region_renderpiece(HiveRegion *region, HivePiece *piece)
{
	static const char *pieceNames[] = {
		[HIVE_ANT] = "A",
		[HIVE_BEETLE] = "B",
		[HIVE_GRASSHOPPER] = "G",
		[HIVE_LADYBUG] = "L",
		[HIVE_MOSQUITO] = "M",
		[HIVE_PILLBUG] = "P",
		[HIVE_QUEEN] = "Q",
		[HIVE_SPIDER] = "S",
	};
	static const char *triangles[] = {
		"\u25e2",
		"\u25e3",
		"\u25e4",
		"\u25e5"
	};
	static const Point cellOffsets[] = {
		{ 0, 0 },
		{ 4, 0 },
		{ 4, 1 },
		{ 0, 1 }
	};
	static const short pairs[3][3] = {
		/* -1 because these values are unused */
		{ -1, -1, -1},
		{ HIVE_PAIR_BLACK,
			HIVE_PAIR_BLACK_BLACK,
			HIVE_PAIR_BLACK_WHITE },
		{ HIVE_PAIR_WHITE,
			HIVE_PAIR_WHITE_BLACK,
			HIVE_PAIR_WHITE_WHITE },
	};
	Point world;
	HivePiece *pieces[6];

	world = piece->position;
	hive_pointtoworld(&world, region->translation);

	wattr_set(region->win, A_REVERSE, (piece->flags & HIVE_SELECTED) ?
		HIVE_PAIR_SELECTED : (piece->flags & HIVE_ISACTOR) ?
		HIVE_PAIR_CHOICE : (piece->side == HIVE_WHITE ?
			HIVE_PAIR_WHITE : HIVE_PAIR_BLACK), NULL);
	mvwprintw(region->win, world.y, world.x + 1, " %s ",
			pieceNames[(int) piece->type]);
	const size_t count = hive_region_countat(region, piece->position);
	if (count > 1)
		mvwprintw(region->win, world.y + 1, world.x + 1, " %zu ", count);
	else
		mvwaddstr(region->win, world.y + 1, world.x + 1, "   ");

	hive_region_getsurroundingr(region, piece->position, pieces);
	for (int n = 0; n < 4; n++) {
		HivePiece *neighbor;

		neighbor = n == 0 ? pieces[HIVE_NORTH_EAST] :
			n == 1 ? pieces[HIVE_NORTH_WEST] :
			n == 2 ? pieces[HIVE_SOUTH_WEST] :
			pieces[HIVE_SOUTH_EAST];
		const int side = neighbor == NULL ? 0 : neighbor->side + 1;
		const short pair = pairs[(int) piece->side + 1][side];
		wattr_set(region->win, 0, pair, NULL);
		const Point off = cellOffsets[n];
		mvwaddstr(region->win, world.y + off.y, world.x + off.x,
				triangles[n]);
	}
}

void hive_region_render(HiveRegion *region)
{
	werase(region->win);
	for (size_t i = 0; i < region->numPieces; i++) {
		HivePiece *const piece = region->pieces[i];
		if (hive_region_getabove(region, piece) == NULL)
			hive_region_renderpiece(region, piece);
	}
	wnoutrefresh(region->win);
}
