#define MAXLENGTH 256
#define MAXNODE 16

struct Node {
	in_addr_t Address;
	int Priority;
	char Status;
};

struct One {
	char Text[MAXLENGTH];
	int Temperature;
	int Light;
	time_t Time;
	char Status;
	in_addr_t Address;
	int Priority;
	int Nodecount;
	struct Node Nodes[MAXNODE];
};

struct Two {
	int Temperature;
	int Light;
	int Priority;
	char Status;
};

stuct Nodestatus {
	int Temperature;
	int Light;
	in_addr_t Address;
	int Priority;
	char Status;
};

struct Status {
	int Nodecount;
	struct Nodestatus Nodes[MAXNODE];
};


