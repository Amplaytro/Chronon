// makemove.c

#include "defs.h"
#include "stdio.h"

#define HASH_PCE(pce,sq) (pos->posKey ^= (PieceKeys[(pce)][(sq)]))
#define HASH_CA (pos->posKey ^= (CastleKeys[(pos->castlePerm)]))
#define HASH_SIDE (pos->posKey ^= (SideKey))
#define HASH_EP (pos->posKey ^= (PieceKeys[EMPTY][(pos->enPas)]))

const int CastlePerm[120] = {
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 13, 15, 15, 15, 12, 15, 15, 14, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15,  7, 15, 15, 15,  3, 15, 15, 11, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15
};

static void ClearPiece(const int sq, S_BOARD *pos) {
	
    int pce = pos->pieces[sq];
	
	int col = PieceCol[pce];
	int index = 0;
	int t_pceNum = -1;
	
    HASH_PCE(pce,sq);
	
	pos->pieces[sq] = EMPTY;
    pos->material[col] -= PieceVal[pce];
	
	if(PieceBig[pce]) {
			pos->bigPce[col]--;
		if(PieceMaj[pce]) {
			pos->majPce[col]--;
		} else {
			pos->minPce[col]--;
		}
	} else {
		CLRBIT(pos->pawns[col], sq);
		CLRBIT(pos->pawns[BOTH], sq);
	}
	
	for(index = 0; index < pos->pceNum[pce]; ++index) {
		if(pos->pList[pce][index] == sq) {
			t_pceNum = index;
			break;
		}
	}
	
	pos->pceNum[pce]--;		
	
	pos->pList[pce][t_pceNum] = pos->pList[pce][pos->pceNum[pce]];
  
}


static void AddPiece(const int sq, S_BOARD *pos, const int pce) {
	
	int col = PieceCol[pce];

    HASH_PCE(pce,sq);
	
	pos->pieces[sq] = pce;

    if(PieceBig[pce]) {
			pos->bigPce[col]++;
		if(PieceMaj[pce]) {
			pos->majPce[col]++;
		} else {
			pos->minPce[col]++;
		}
	} else {
		SETBIT(pos->pawns[col], sq);
		SETBIT(pos->pawns[BOTH], sq);
	}
	
	pos->material[col] += PieceVal[pce];
	pos->pList[pce][pos->pceNum[pce]++] = sq;
	
}

static void MovePiece(const int from, const int to, S_BOARD *pos) {
	
	int index = 0;
	int pce = pos->pieces[from];	
	int col = PieceCol[pce];

	HASH_PCE(pce,from);
	pos->pieces[from] = EMPTY;
	
	HASH_PCE(pce,to);
	pos->pieces[to] = pce;
	
	if(!PieceBig[pce]) {
		CLRBIT(pos->pawns[col], from);
		CLRBIT(pos->pawns[BOTH], from);
		SETBIT(pos->pawns[col], to);
		SETBIT(pos->pawns[BOTH], to);		
	}    
	
	for(index = 0; index < pos->pceNum[pce]; ++index) {
		if(pos->pList[pce][index] == from) {
			pos->pList[pce][index] = to;
			break;
		}
	}
}

int MakeMove(S_BOARD *pos, int move) {
	
	int from = FROMSQ(move);
    int to = TOSQ(move);
    int side = pos->side;
	
	pos->history[pos->hisPly].posKey = pos->posKey;
	
	if(move & MFLAGEP) {
        if(side == WHITE) {
            ClearPiece(to-10,pos);
        } else {
            ClearPiece(to+10,pos);
        }
    } else if (move & MFLAGCA) {
        switch(to) {
            case C1:
                MovePiece(A1, D1, pos);
			break;
            case C8:
                MovePiece(A8, D8, pos);
			break;
            case G1:
                MovePiece(H1, F1, pos);
			break;
            case G8:
                MovePiece(H8, F8, pos);
			break;
            default: break;
        }
    }	
	
	if(pos->enPas != NO_SQ) HASH_EP;
    HASH_CA;
	
	pos->history[pos->hisPly].move = move;
    pos->history[pos->hisPly].fiftyMove = pos->fiftyMove;
    pos->history[pos->hisPly].enPas = pos->enPas;
    pos->history[pos->hisPly].castlePerm = pos->castlePerm;

    pos->castlePerm &= CastlePerm[from];
    pos->castlePerm &= CastlePerm[to];
    pos->enPas = NO_SQ;

	HASH_CA;
	
	int captured = CAPTURED(move);
    pos->fiftyMove++;
	
	if(captured != EMPTY) {
        ClearPiece(to, pos);
        pos->fiftyMove = 0;
    }
	
	pos->hisPly++;
	pos->ply++;
	
	if(PiecePawn[pos->pieces[from]]) {
        pos->fiftyMove = 0;
        if(move & MFLAGPS) {
            if(side==WHITE) {
                pos->enPas=from+10;
            } else {
                pos->enPas=from-10;
            }
            HASH_EP;
        }
    }
	
	MovePiece(from, to, pos);
	
	int prPce = PROMOTED(move);
    if(prPce != EMPTY)   {
        ClearPiece(to, pos);
        AddPiece(to, pos, prPce);
    }
	
	if(PieceKing[pos->pieces[to]]) {
        pos->KingSq[pos->side] = to;
    }
	
	pos->side ^= 1;
    HASH_SIDE;
	
		
	if(SqAttacked(pos->KingSq[side],pos->side,pos))  {
        TakeMove(pos);
        return FALSE;
    }
	
	return TRUE;
	
}

void TakeMove(S_BOARD *pos) {
	
	pos->hisPly--;
    pos->ply--;
	
    int move = pos->history[pos->hisPly].move;
    int from = FROMSQ(move);
    int to = TOSQ(move);	

	
	if(pos->enPas != NO_SQ) HASH_EP;
    HASH_CA;

    pos->castlePerm = pos->history[pos->hisPly].castlePerm;
    pos->fiftyMove = pos->history[pos->hisPly].fiftyMove;
    pos->enPas = pos->history[pos->hisPly].enPas;

    if(pos->enPas != NO_SQ) HASH_EP;
    HASH_CA;

    pos->side ^= 1;
    HASH_SIDE;
	
	if(MFLAGEP & move) {
        if(pos->side == WHITE) {
            AddPiece(to-10, pos, bP);
        } else {
            AddPiece(to+10, pos, wP);
        }
    } else if(MFLAGCA & move) {
        switch(to) {
            case C1: MovePiece(D1, A1, pos); break;
            case C8: MovePiece(D8, A8, pos); break;
            case G1: MovePiece(F1, H1, pos); break;
            case G8: MovePiece(F8, H8, pos); break;
            default: break;
        }
    }
	
	MovePiece(to, from, pos);
	
	if(PieceKing[pos->pieces[from]]) {
        pos->KingSq[pos->side] = from;
    }
	
	int captured = CAPTURED(move);
    if(captured != EMPTY) {
        AddPiece(to, pos, captured);
    }
	
	if(PROMOTED(move) != EMPTY)   {
        ClearPiece(from, pos);
        AddPiece(from, pos, (PieceCol[PROMOTED(move)] == WHITE ? wP : bP));
    }
}

















