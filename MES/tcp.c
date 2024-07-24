/****************************************/
/* MES/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/*  Modified By				*/
/*     T.Mochizuki 04/03/09             */
/****************************************/
#include <string.h>
#include "../syscall.h"
#include "task.h"
#include "ip.h"
#define SuperPID 1
#define TCP_NUM		32
#define TCP_DEF_MTU	1460

#define MAX_TCPDATA_ITEM 10
#define MAX_TCPDATA_MEM  4500
#define IS_TCP_DATALIST_ITEM_USED(x) ((x).byStat & 0x01 )
#define IS_TCP_DATALIST_ITEM_ACKED(x) ((x).byStat & 0x02 )
#define IS_TCP_DATALIST_ITEM_COMPLETE(x) ((x).byStat & 0x04 )
#define IS_TCP_DATALIST_ITEM_READ(x) ((x).byStat & 0x08 )
#define SET_TCP_DATALIST_ITEM_USE(x) ((x).byStat |= 0x01 )
#define SET_TCP_DATALIST_ITEM_ACKED(x) ((x).byStat |= 0x02 )
#define SET_TCP_DATALIST_ITEM_COMPLETE(x) ((x).byStat |= 0x04 )
#define SET_TCP_DATALIST_ITEM_READ(x) ((x).byStat |= 0x08 )


extern MemInfo	meminfo, tmpmem;
extern int32_t	sys_count;
extern Task	*curtask;
uint16_t make_crc(STRING, int32_t);
uint32_t get_ip(int32_t);
Task* get_taskptr(int32_t);
static int32_t tcp_out(uint32_t);

typedef struct {
	uint32_t src_ip	__attribute__((packed));
	uint32_t dst_ip	__attribute__((packed));
	Byte	 zero	__attribute__((packed));
	Byte	 proto	__attribute__((packed));
	uint16_t tcpsiz	__attribute__((packed));
} pseudo_hdr;

typedef struct _TCPDataListItem {
  Byte    byStat;  // 0bit Use, 1bit Return ACK,2bit Complete Sequence,3bit Data Read
  uint32_t ulSize;
  uint32_t ulSeqNext;
  struct _TCPDataListItem *pBefore;
  struct _TCPDataListItem *pNext;
  STRING pData;
} TCPDataListItem;
typedef struct _TCPDataList {
  uint16_t usCount;
  uint32_t ulCompleteSeqNext;
  struct _TCPDataListItem *pFirst;
  struct _TCPDataListItem *pLast;
  struct _TCPDataListItem *pReading;
  uint32_t ulReadPos;
  uint32_t ulTotalSize;
  TCPDataListItem Item[MAX_TCPDATA_ITEM];
} TCPDataList;

typedef struct {
	uint32_t rem_ip;	// Remote IP address
	uint32_t send_unacked;
	uint32_t send_next;
	uint32_t receive_next;
	uint32_t packet_size;
	uint32_t next_packet_size;
	uint32_t recv_packet_size;
	STRING	 packet;
	STRING	 next_packet;
	STRING	 recv_packet;
	int32_t  (*event_listener)(uint32_t, Byte, uint32_t, uint16_t);
	uint16_t remport;	// Remote TCP port
	uint16_t locport;	// Local TCP port
	uint16_t id;		// task id
	uint16_t send_mtu;
	Byte	state;
	Byte	type;
	Byte	flags;		// State machine flags
	Byte	myflags;	// My flags to be Txed
	Byte	close_count;
	Byte	resend_timer;
	Byte	ack_timer;
	Byte	fin_timer;
        TCPDataList *pRDataList;
} tcp_info;
//static tcp_info tcp_socket[TCP_NUM + 1];
static tcp_info *tcp_socket;

TCPDataListItem *FindUnuseTCPDataListItem(TCPDataList *pDList)
{
  int16_t i;
  for ( i = 0 ; i < MAX_TCPDATA_ITEM;i++ ) {
    if ( IS_TCP_DATALIST_ITEM_USED(pDList->Item[i]) == 0 ) return &(pDList->Item[i]);
  }
  return 0;
}
// Release TCPDataListItem
// return next Item
TCPDataListItem *ReleaseTCPDataListItem(TCPDataList *pDList,TCPDataListItem *pDLItem)
{
  TCPDataListItem *pBDLItem;
  TCPDataListItem *pNDLItem;
  if ( pDLItem == 0 ) return;
  if ( pDLItem->pData != 0 ) {
    free_mem(meminfo, SuperPID, pDLItem->pData );
    pDLItem->pData  = 0;
    pBDLItem = pDLItem->pBefore;
    pNDLItem = pDLItem->pNext;
    if ( pDLItem == pDList->pReading ) {
      pDList->ulReadPos = 0;
      pDList->pReading = 0;
    }
    if ( pBDLItem == 0 ) {
      pDList->pFirst = pNDLItem;
    } else {
      pBDLItem->pNext = pNDLItem;
    }
    if ( pNDLItem == 0 ) {
      pDList->pLast = pBDLItem;
    } else {
      pNDLItem->pBefore = pBDLItem;
    }
    pDList->usCount--;
    pDList->ulTotalSize -= pDLItem->ulSize;
  }
  memset((void*)pDLItem,0,sizeof(TCPDataListItem)); // 0 clear
  return pNDLItem;
}
// Add TCPDataListItem to TCPDataList
// pDList      : TCP Data List
// ulRSeqNext  : next seq number
// ulSize      : Data Size
// pData       : Data
// return      : Added TCP Data List Item( if 0 ,not add)
TCPDataListItem *AddTCPDataListItem(TCPDataList *pDList,uint32_t ulRSeqNext,uint32_t ulSize,STRING pData)
{
  int16_t i;
  TCPDataListItem *pDLItem;
  TCPDataListItem *pDLItem2;
  uint32_t ulSeqNext2;
  int iRdflg;

  if ( ulSize == 0 )  return 0; 
  if ( pDList->pLast != 0 && pDList->usCount >= MAX_TCPDATA_ITEM) {
    if (    pDList->pLast->ulSeqNext > ulRSeqNext  && pDList->ulCompleteSeqNext < ulRSeqNext  )  
      ReleaseTCPDataListItem(pDList,pDList->pLast);
  }
  if ( pDList->usCount >= MAX_TCPDATA_ITEM  // no more TCPDataListItem
       || pDList->ulTotalSize + ulSize >= MAX_TCPDATA_MEM  // no more memory
       ) {
    return 0; 
  }
  if ( pDList->ulCompleteSeqNext >= ulRSeqNext ) return 0;
  if ( ( pDLItem= FindUnuseTCPDataListItem(pDList) ) == 0 ) return 0; // Find unuse Item 
  if ( pDList->pFirst == 0 )  {
    pDList->pFirst = pDList->pLast = pDLItem;// First Add
    pDLItem->ulSeqNext = ulRSeqNext;
  } else if ( ulRSeqNext > pDList->pLast->ulSeqNext ) { // This socket set in DataList as  Last Item 
    pDLItem->pBefore = pDList->pLast;
    pDList->pLast = pDList->pLast->pNext = pDLItem;
  } else if ( ulRSeqNext < pDList->pFirst->ulSeqNext ) { // This socket set in DataList as  First Item 
    pDLItem->pNext = pDList->pFirst;
    pDList->pFirst = pDList->pFirst->pBefore = pDLItem;
  } else { // This socket set in DataList between First and Last Item 
    for ( pDLItem2 = pDList->pFirst; pDLItem2->pNext != 0; pDLItem2 = pDLItem2->pNext ) {
      if ( ulRSeqNext < pDLItem2->pNext->ulSeqNext && ulRSeqNext > pDLItem2->ulSeqNext) { 
	pDLItem2->pNext->pBefore  = pDLItem;
	pDLItem->pNext = pDLItem2->pNext;
	pDLItem2->pNext = pDLItem;
	pDLItem->pBefore = pDLItem2;
	break;
      }
    }
    if ( pDLItem2->pNext == 0 ) return 0;
  }

  pDLItem->pData = (STRING)alloc_mem(meminfo, SuperPID, ulSize);
  if ( pDLItem->pData == 0 ) return 0;
  pDLItem->ulSize = ulSize;
  SET_TCP_DATALIST_ITEM_USE(*pDLItem);

  pDLItem->ulSeqNext = ulRSeqNext;
  ulSeqNext2 = pDList->ulCompleteSeqNext;
  memcpy(pDLItem->pData,pData,ulSize);
  for ( pDLItem2 = pDList->pFirst; pDLItem2 != 0; pDLItem2 = pDLItem2->pNext ) {
    if ( IS_TCP_DATALIST_ITEM_COMPLETE(*pDLItem2) == 0 ) {
      if ( ulSeqNext2 == ( pDLItem2->ulSeqNext - pDLItem2->ulSize )) {
	SET_TCP_DATALIST_ITEM_COMPLETE(*pDLItem2);
      } else break;
    }
    ulSeqNext2 = pDLItem2->ulSeqNext;
  }
  pDList->usCount++;
  pDList->ulCompleteSeqNext = ulSeqNext2;
  pDList->ulTotalSize += ulSize;
  return pDLItem;
}
// TPCDataListから読み込み済みの不要なデータを削除する
void GCTCPDataList(TCPDataList *pDList)
{
  TCPDataListItem *pDLItem;
  for ( pDLItem = pDList->pFirst; pDLItem != 0; ) {
    if ( IS_TCP_DATALIST_ITEM_READ(*pDLItem) ) {
      pDLItem = ReleaseTCPDataListItem(pDList,pDLItem);
    } else {
      break;
    }
  }
}
// Set ACK Flag to TCPDataList
void SetTCPSendAck(TCPDataList *pDList,uint32_t ulAckSeq)
{
  TCPDataListItem *pDLItem;
  for ( pDLItem = pDList->pFirst; pDLItem != 0; pDLItem = pDLItem->pNext ) {
    if ( pDLItem->ulSeqNext <= ulAckSeq ) {
      SET_TCP_DATALIST_ITEM_ACKED(*pDLItem);
      break;
    }
  }
}
// Init TCPSocket for TCPDataList
void InitTCPSock(tcp_info *pSock,uint32_t ulSeqNext) // This function must be called in event flag = TCP_EVENT_CONNECTED
{
  TCPDataList *pDList;
  pDList = pSock->pRDataList = (TCPDataList*)alloc_mem(meminfo, SuperPID, sizeof(TCPDataList));
  memset((void*)pDList,0,sizeof(TCPDataList)); // 0 clear
  pDList->ulCompleteSeqNext = ulSeqNext;
}
// Release TCPSocket for TCPDataList
void ReleaseTCPSock(tcp_info* pSock) // This function must be called in event flag = TCP_EVENT_CLOSE or ABORT
{
  int i;
  TCPDataList *pDList;
  if ( ( pDList = pSock->pRDataList ) == 0 ) return;
  for ( i = 0 ; i < MAX_TCPDATA_ITEM;i++ ) {
    if ( pDList->Item[i].pData != 0 ) {
      free_mem(meminfo, SuperPID, pDList->Item[i].pData );
      pDList->Item[i].pData = 0;
    }
  }
  free_mem(meminfo, SuperPID, (STRING)pSock->pRDataList );
  pSock->pRDataList = 0;
}
// Set TCP Data to TCPDataList
int32_t SetTCPRData(tcp_info* pSock,STRING pData,uint32_t ulSize)
{
  TCPDataListItem *pDLItem;
  GCTCPDataList(pSock->pRDataList);
  pDLItem = AddTCPDataListItem(pSock->pRDataList,pSock->receive_next+ulSize,ulSize,pData);
  if ( pDLItem == 0 ) return -1;
  return 0;
}
// Read TCP Data from TCPDataList
uint32_t ReadTCPRData(tcp_info* pSock,STRING pData,uint32_t ulSize)
{
  TCPDataList *pDList;
  TCPDataListItem *pDLItem,*pNDLItem;
  uint32_t ulSz,ulPos,ulSz2,ulPos2,ulS;
  pDList = pSock->pRDataList;
  if ( pDList == 0 ) return -1;
  if ( (pDLItem = pDList->pReading) == 0 )  {
    for ( pDLItem = pDList->pFirst; pDLItem != 0 ; pDLItem = pDLItem->pNext ) {
      if ( IS_TCP_DATALIST_ITEM_COMPLETE(*pDLItem) != 0 && IS_TCP_DATALIST_ITEM_READ(*pDLItem) == 0 ) break;
    }
  }
  if ( pDLItem == 0 ) return 0;
  ulSz = ulSize;
  ulPos = 0;
  ulPos2 = pDList->ulReadPos;
  do {
    if ( ulSz == 0 ) {
      pSock->pRDataList->ulReadPos = ulPos2;
      pSock->pRDataList->pReading = pDLItem;
      return ulPos;
    }
    ulSz2 = pDLItem->ulSize - ulPos2;
    if ( ulSz2 <= ulSz ) ulS = ulSz2;
    else ulS = ulSz;
    memcpy(&pData[ulPos],&(pDLItem->pData[ulPos2]),ulS);
    ulPos += ulS;
    ulSz -= ulS;
    ulPos2 += ulS;
    if ( ulSz < 0 ) {
      pSock->pRDataList->ulReadPos = ulPos2;
      pSock->pRDataList->pReading = pDLItem;
      return ulPos;
    } else if ( ulSz > 0 || ( ulSz == 0 && ulPos2 == pDLItem->ulSize)) {
      ulPos2 = 0;
      SET_TCP_DATALIST_ITEM_READ(*pDLItem);
      pDLItem = pDLItem->pNext;
    } 
    if (pDLItem == 0 ) break;
  } while(IS_TCP_DATALIST_ITEM_COMPLETE(*pDLItem) != 0 );
  pSock->pRDataList->pReading = 0;
  pSock->pRDataList->ulReadPos = 0;
  return ulPos;
}

int hex_write(char *str, int size) {
	const unsigned char hextbl[] = "0123456789ABCDEF";
	unsigned int	i, val;
	unsigned char	buf[2];

	for(i = 0;i < size;i++) {
		val = (int)(str[i]) & 0xff;
		buf[0] = hextbl[(val >> 4) & 0x0f];
		buf[1] = hextbl[val & 0x0f];
		write_device(0x11, buf, 2);
	}
}

void tcp_init() {
	int32_t  i;
	tcp_socket = (tcp_info *)alloc_mem(meminfo, SuperPID,sizeof(tcp_info)*(TCP_NUM + 1));
	memset(tcp_socket,0,sizeof(tcp_info)*(TCP_NUM+1));
	for(i = 0;i <= TCP_NUM;i++) {
		tcp_socket[i].state = TCP_STATE_FREE;
		tcp_socket[i].type = TCP_TYPE_NONE;
		tcp_socket[i].send_mtu = TCP_DEF_MTU;
	}
}

int32_t tcp_setid(uint16_t id, uint32_t sock) {
	tcp_info *soc;
	
	if(sock >= TCP_NUM) return -1;
	soc = &tcp_socket[sock];
	soc->id = id;
	return 0;
}

static int32_t tcp_getsocket(int16_t id, int32_t (*listener)(uint32_t, Byte, uint32_t, uint16_t)) {
	int32_t  i;
	tcp_info *soc;
	
	if(listener == 0) return -1;
	for(i = 0;i < TCP_NUM;i++) {
		soc = &tcp_socket[i];
		if(soc->state == TCP_STATE_FREE) {
		        memset(soc,0,sizeof(tcp_info));
			soc->state = TCP_STATE_RESERVED;
			soc->type = TCP_TYPE_NONE;
			soc->event_listener = listener;
			soc->id = id;
			return i;
		}
	}
	return -1;
}

static int32_t tcp_dupsocket(int32_t sock) {
	int32_t  i;
	tcp_info *dest, *src;

	if(sock >= TCP_NUM) return -1;
	src = &tcp_socket[sock];
	for(i = 0;i < TCP_NUM;i++) {
		dest = &tcp_socket[i];
		if(dest->state == TCP_STATE_FREE) {
			memcpy((STRING)dest, (STRING)src, sizeof(tcp_info));
			return i;
		}
	}
	return -1;
}

static int32_t tcp_releasesocket(int16_t id, uint32_t sock) {
	tcp_info *soc;
	
	soc = &tcp_socket[sock];
	if((soc->id != id) &&
	   (soc->state != TCP_STATE_FREE) &&
	   (soc->state != TCP_STATE_RESERVED) &&
	   (soc->state != TCP_STATE_CLOSED)) {
		return -1;
	}
	ReleaseTCPSock(soc); // Add T.Mochizuki
	soc->state = TCP_STATE_FREE;
	soc->type = TCP_TYPE_NONE;
	soc->event_listener = 0;
	soc->rem_ip = 0;
	soc->remport = 0;
	soc->locport = 0;
	soc->flags = 0;
	soc->packet = 0;
	soc->resend_timer = 0;
	soc->ack_timer = 0;
	soc->fin_timer = 0;
	soc->close_count = 0;
	soc->id = 0;
	soc->recv_packet_size = 0;
	return sock;
}

static int32_t tcp_listen(int16_t id, uint32_t sock, uint16_t port) {
	tcp_info *soc;
	int32_t	 i;

	if(sock >= TCP_NUM) return -1;
	soc = &tcp_socket[sock];
	for(i = 0;i < TCP_NUM;i++) {
		if(tcp_socket[i].id != 0 &&
		   tcp_socket[i].type == TCP_TYPE_SERVER &&
		   tcp_socket[i].locport == port) {
			return -1;
		}
	}
	if(soc->event_listener == 0) return -1;
	if((soc->state != TCP_STATE_RESERVED) &&
	   (soc->state != TCP_STATE_LISTENING)	&&
	   (soc->state != TCP_STATE_CLOSED) &&		
	   (soc->state != TCP_STATE_TIMED_WAIT)) {
		return -1;
	}
	soc->id = id;
	soc->type = TCP_TYPE_SERVER;
	soc->state = TCP_STATE_LISTENING;
	soc->locport = port;
	soc->send_next = 0xFFFFFFFF;
	soc->send_mtu = TCP_DEF_MTU;

	soc->flags = 0;
	soc->rem_ip = 0;
	soc->remport = 0;
	soc->send_unacked = 0;
	soc->myflags = 0;
	soc->receive_next = 0;
	return sock;
}

static uint16_t tcp_getfreeport() {
	static uint16_t lastport = 1;
	tcp_info *soc;
	uint16_t start, i;
	
	for(start = lastport++;start != lastport;lastport++) {
		if(lastport == TCP_PORTS_END) lastport = 1;
		for(i = 0;i < TCP_NUM;i++) {
			soc = &tcp_socket[i];
			if((soc->state > TCP_STATE_CLOSED) && (soc->locport == lastport)) {
				/* This socket has reserved the port, go to next one	*/
				break; 
			}
		}	
		if(i == TCP_NUM) break;
	}
	if(lastport == start) return 0;
	return lastport;
}

static void tcp_newstate(int32_t sock, Byte nstate){
	tcp_info *soc;
	Task	 *task;

	soc = &tcp_socket[sock];
	if(nstate == TCP_STATE_TIMED_WAIT && soc->state != TCP_STATE_TIMED_WAIT) {
		soc->close_count = 1;
	}
	if(soc->state == TCP_STATE_CONNECTED && nstate != TCP_STATE_CONNECTED) {
		if(soc->recv_packet_size > 0) {
			free_mem(meminfo, SuperPID, soc->recv_packet);
			soc->recv_packet_size = 0;
		}
	}
	soc->state = nstate;
	if(soc->state == TCP_STATE_CONNECTED && soc->resend_timer > 0) {
		soc->resend_timer = 0;
		free_mem(meminfo, SuperPID, soc->packet);
		task = get_taskptr(soc->id);
		if(task->req == TCP_SEND_REQ && task->state & NET_STATE) {
			task->state &= ~NET_STATE;
			task->retval = soc->packet_size = soc->next_packet_size;
			soc->packet = alloc_mem(meminfo, SuperPID, soc->packet_size);
			memcpy(soc->packet, soc->next_packet, soc->packet_size);
			soc->send_next += soc->packet_size;
			soc->resend_timer = TCP_DEF_RETRIES;
			tcp_out(sock);
		}
	}
	if(soc->state == TCP_STATE_CLOSED && soc->fin_timer > 0) {
		soc->fin_timer = 0;
		task = get_taskptr(soc->id);
		if(task->req == TCP_FREE_REQ && task->state & NET_STATE) {
			task->state &= ~NET_STATE;
			tcp_releasesocket(soc->id, sock);
			task->retval = sock;
		}
	}
	return;
}

Byte tcp_getstate(uint32_t s) {
	int32_t	 sock;
	tcp_info *soc;

	sock = s & SOCK_MASK;
	if(sock >= TCP_NUM) return -1;
	soc = &tcp_socket[sock];
	return soc->state;
}

static uint32_t tcp_initseq(){
	return (sys_count << 16) | 0x0000FFFF;
}

static int32_t process_tcp_out(int32_t sock, STRING data, int32_t tcpsize) {
	int32_t	 index, fd, pktsize, size, optsize;
	tcp_info *soc;
	TCP_HDR	 *tcphdr;
	Ether	 *ether_buf;
	pseudo_hdr *pseudo;
	STRING	 ptr, packet;
	int32_t	 *tcp_opt;
	uint16_t checksum;

	if(sock >= TCP_NUM) return -1;
	soc = &tcp_socket[sock];
	optsize	= (soc->myflags & TCP_FLAG_SYN) ? 8 : 0;
	pktsize	= sizeof(Ether) + sizeof(IP_HDR) + sizeof(TCP_HDR) + optsize + tcpsize;
	packet = alloc_mem(tmpmem, SuperPID, pktsize);
	if(packet == 0) return -1;
	bzero(packet, pktsize - tcpsize);
	ptr = &packet[pktsize - tcpsize];
	if(tcpsize > 0) memcpy(ptr, data, tcpsize);
	pseudo = (pseudo_hdr*)&packet[sizeof(Ether) + sizeof(IP_HDR) - sizeof(pseudo_hdr)];
	ether_buf = (Ether*)packet;
	ptr = (STRING)&ether_buf[1];
	ptr = &(ptr[sizeof(IP_HDR)]);
	tcphdr = (TCP_HDR*)ptr;
	tcp_opt = (int32_t*)&(ptr[sizeof(TCP_HDR)]);
	for(index = 0;index < IPNUM;index++) {
		if((fd = index2fd(index)) != -1) {
			pseudo->src_ip = get_ip(index);
			pseudo->dst_ip = soc->rem_ip;
			pseudo->zero = 0;
			pseudo->proto = TCP;
			pseudo->tcpsiz = sizeof(TCP_HDR) + optsize + tcpsize;
			tcphdr->sport = soc->locport;
			tcphdr->dport = soc->remport;
			tcphdr->seqno = soc->send_unacked;
			tcphdr->ackno = soc->receive_next;
			tcphdr->hlen_flags = ((sizeof(TCP_HDR) + optsize) / 4) << 12;
			tcphdr->hlen_flags |= soc->myflags;
			tcphdr->window = TCP_DEF_MTU;
			tcphdr->checksum = 0;
			if(soc->myflags & TCP_FLAG_SYN) {
				tcp_opt[0] = TCP_DEF_MTU;
				tcp_opt[1] = 0x01010402;
			}
			tcphdr->checksum = make_crc((STRING)pseudo, sizeof(pseudo_hdr) + pseudo->tcpsiz);
			ip_opt_header(ether_buf, 0, 0);
			size = ip_header(fd, ether_buf, soc->rem_ip, TCP, sizeof(TCP_HDR) + optsize + tcpsize);
			write_ip_packet(fd, (STRING)ether_buf, pktsize);
		}
	}
	free_mem(tmpmem, SuperPID, packet);
	return tcpsize;
}

static void tcp_sendcontrol(uint32_t sock) {
	int32_t i;

	if(sock > TCP_NUM) return;
	process_tcp_out(sock, 0, 0);
	return;
}

static int32_t tcp_out(uint32_t sock) {
	Task	 *task;
	tcp_info *soc;

	soc = &tcp_socket[sock];
	soc->myflags = TCP_FLAG_ACK | TCP_FLAG_PUSH;
	process_tcp_out(sock, soc->packet, soc->packet_size);
	return soc->packet_size;
}

static int32_t tcp_checksend(uint32_t sock){
	tcp_info *soc;

	if(sock >= TCP_NUM) return -1;
	soc = &tcp_socket[sock];
	if(soc->state != TCP_STATE_CONNECTED) return -1;
	if(soc->send_unacked == soc->send_next) return soc->send_mtu;
	return -1;
}

static void tcp_sendreset(TCP_HDR *tcphdr, uint32_t remip) {
	tcp_info *soc;

	soc = &tcp_socket[TCP_NUM];
	if(tcphdr->hlen_flags & TCP_FLAG_RESET) return;
	soc->rem_ip = remip;
	soc->remport = tcphdr->sport;
	soc->locport = tcphdr->dport;
	if(tcphdr->hlen_flags & TCP_FLAG_ACK ) {
		soc->send_unacked = tcphdr->ackno;
		soc->myflags = TCP_FLAG_RESET;	
		soc->receive_next = tcphdr->seqno;
	} else {
		soc->send_unacked = 0;
		soc->myflags = TCP_FLAG_RESET | TCP_FLAG_ACK;	
		soc->receive_next = tcphdr->seqno + 1;
	}
	soc->send_mtu = TCP_DEF_MTU;
	tcp_sendcontrol(TCP_NUM);
}

static uint32_t tcp_abort(uint32_t sock) {
	tcp_info *soc;

	if(sock >= TCP_NUM) return -1;
	soc = &tcp_socket[sock];
	switch(soc->state) {
		case TCP_STATE_FREE:
			return -1;
		case TCP_STATE_RESERVED:
		case TCP_STATE_CLOSED:
			return sock;
		case TCP_STATE_TIMED_WAIT:
		case TCP_STATE_LISTENING:
			tcp_newstate(sock, TCP_STATE_CLOSED);
			return sock;
		case TCP_STATE_SYN_SENT:
		case TCP_STATE_SYN_RECEIVED:
		case TCP_STATE_CONNECTED:
		case TCP_STATE_FINW1:
		case TCP_STATE_FINW2:
		case TCP_STATE_CLOSING:
		case TCP_STATE_LAST_ACK:
			soc->myflags = TCP_FLAG_RESET;
			tcp_sendcontrol(sock);
			tcp_newstate(sock, TCP_STATE_CLOSED);
			return sock;
		default:
			return -1;
	}
}

int32_t tcpbind(int16_t id, uint32_t s, struct sockaddr *addr) {
	tcp_info *soc;
	int32_t	 sock;

	sock = s & SOCK_MASK;
	if(sock >= TCP_NUM) return -1;
	soc = &tcp_socket[sock];
	if(soc->event_listener == 0) return -1;
	soc->id = id;
	soc->locport = addr->sin_port;
	return 0;
}

int32_t tcpconnect(int16_t id, uint32_t s, struct sockaddr *addr) {
	tcp_info *soc;
	int16_t	 myport;
	int32_t	 sock;
	uint32_t ip;

	sock = s & SOCK_MASK;
	if(sock >= TCP_NUM) return -1;
	soc = &tcp_socket[sock];
	soc->id =id;
	if(soc->locport == 0) {
		myport = tcp_getfreeport();
		if(myport == 0) return -1;
		soc->locport = myport;
	}
	if(soc->event_listener == 0) return -1;
	if((soc->state != TCP_STATE_RESERVED) &&
	   (soc->state != TCP_STATE_LISTENING) &&
	   (soc->state != TCP_STATE_CLOSED)) {
		return -1;
	}
	soc->type = TCP_TYPE_CLIENT;
	soc->rem_ip = addr->sin_addr;
	soc->remport = addr->sin_port;
	soc->flags = 0;
	soc->send_mtu = TCP_DEF_MTU;
	
	soc->send_unacked = tcp_initseq(); 
	soc->send_next = soc->send_unacked + 1;
	soc->myflags = TCP_FLAG_SYN;
	tcp_sendcontrol(sock);
	tcp_newstate(sock, TCP_STATE_SYN_SENT);
	
	return 0;
}

static uint32_t tcp_close(int16_t id, uint32_t sock){
	tcp_info *soc;

	if(sock >= TCP_NUM) return -1;
	soc = &tcp_socket[sock];
	soc->id = id;
	switch(soc->state) {
		case TCP_STATE_LISTENING:
			tcp_newstate(sock, TCP_STATE_CLOSED);
			break;
		case TCP_STATE_SYN_RECEIVED:
			soc->myflags = TCP_FLAG_ACK | TCP_FLAG_FIN;
			soc->send_unacked++;
			soc->send_next++;
			tcp_sendcontrol(sock);
			tcp_newstate(sock, TCP_STATE_FINW1);
			break;
		case TCP_STATE_SYN_SENT:
			tcp_newstate(sock, TCP_STATE_CLOSED);
			break;
		case TCP_STATE_FINW1:
		case TCP_STATE_FINW2:
		case TCP_STATE_CLOSING:
		case TCP_STATE_TIMED_WAIT:
		case TCP_STATE_LAST_ACK:
			break;
		case TCP_STATE_CONNECTED:
			if(soc->send_unacked == soc->send_next ) {
				/* There is no unacked data	*/
				soc->myflags = TCP_FLAG_ACK | TCP_FLAG_FIN;
				soc->send_next++;
				tcp_sendcontrol(sock);
				tcp_newstate(sock, TCP_STATE_FINW1);				
			} else {
				/* Can't do much but raise pollable flag to soc->flags	*/
				/* and process it on tcp_poll				*/
				soc->flags |= TCP_INTFLAGS_CLOSEPENDING;
				return sock;
			}
			break;
		default:
			return -1;
	}
	return sock;
}

static int32_t tcp_mapsocket(IP_HDR *iphdr) {
	TCP_HDR	 *tcphdr;
	STRING	 ptr;
	int32_t  i;

	ptr = (STRING)iphdr;
	ptr = &(ptr[iphdr->header_len * 4]);
	tcphdr = (TCP_HDR*)ptr;
	for(i = 0;i < TCP_NUM;i++) {
		if(tcp_socket[i].state == TCP_STATE_LISTENING) continue;
		if(tcp_socket[i].remport != tcphdr->sport) continue;
		if(tcp_socket[i].locport != tcphdr->dport) continue;
		if(tcp_socket[i].rem_ip != iphdr->srcip) continue;
		return i;
	}
	if((tcphdr->hlen_flags & TCP_FLAG_SYN) == 0) return -1;
	if(tcphdr->hlen_flags & TCP_FLAG_ACK) return -1;
	if(tcphdr->hlen_flags & TCP_FLAG_RESET) return -1;
	if(tcphdr->hlen_flags & TCP_FLAG_FIN) return -1;
	for(i = 0;i < TCP_NUM;i++){
		if(tcp_socket[i].state != TCP_STATE_LISTENING) continue;	
		if(tcp_socket[i].locport != tcphdr->dport) continue;
		tcp_socket[i].rem_ip = iphdr->srcip;
		tcp_socket[i].remport = tcphdr->sport;
		return i;
	}
	return -1;
}

int32_t tcp(uint32_t fd, IP_HDR *iphdr){
	tcp_info *soc;
	TCP_HDR	 *tcphdr;
	STRING	 ptr, tcpdata;
	int32_t	 i, len, diff, sock;
	uint16_t hlen, dlen;
	int16_t	 temp;
	Byte	 olen;
	
	ptr = (STRING)iphdr;
	ptr = &(ptr[iphdr->header_len * 4]);
	tcphdr = (TCP_HDR*)ptr;
	len = iphdr->datasize;
	len -= (int32_t)iphdr->header_len * 4;
	hlen = tcphdr->hlen_flags & 0xF000;
	hlen >>= 10;
	if(hlen < MIN_TCP_HLEN) return -1;
	olen = hlen - MIN_TCP_HLEN;
	if(olen > MAX_TCP_OPTLEN) return -1;
	if(hlen > len) return -1;
	dlen = len - hlen;
	tcpdata = &ptr[hlen];
	sock = tcp_mapsocket(iphdr);
	if(sock == -1) return 0;
	soc = &tcp_socket[sock];
	switch(soc->state) {
		case TCP_STATE_CONNECTED:
			if(tcphdr->hlen_flags & TCP_FLAG_RESET)	{
				soc->event_listener(sock, TCP_EVENT_ABORT, soc->rem_ip, soc->remport);
				if(soc->type & TCP_TYPE_SERVER)
					tcp_newstate(sock, TCP_STATE_LISTENING);
				else
					tcp_newstate(sock, TCP_STATE_CLOSED);
				return -1;
			}
			if( tcphdr->hlen_flags & TCP_FLAG_SYN )	{
				if( tcphdr->hlen_flags & TCP_FLAG_ACK )	{
					if( (tcphdr->seqno + 1) == soc->receive_next ) {
						if( tcphdr->ackno == soc->send_next ) {
							soc->myflags = TCP_FLAG_ACK;
							tcp_sendcontrol(sock);
							return 0;
						}
					}
				 return 0;
				}
			}
			if(soc->send_unacked != soc->send_next) {
				if((tcphdr->hlen_flags & TCP_FLAG_ACK) == 0) return 0;
				if(tcphdr->ackno == soc->send_next) {
					soc->send_unacked = soc->send_next;
					soc->event_listener(sock, TCP_EVENT_ACK, soc->rem_ip, soc->remport);
				}
			}
			if(soc->receive_next != tcphdr->seqno) {
				soc->myflags = TCP_FLAG_ACK;
				tcp_sendcontrol(sock);
				return 0;
			}
			if(dlen) soc->event_listener(sock, TCP_EVENT_DATA, (uint32_t)tcpdata, dlen);
			//soc->receive_next += dlen; 
			soc->receive_next = soc->pRDataList->ulCompleteSeqNext; // Mod T.Mochizuki
			if( tcphdr->hlen_flags & TCP_FLAG_FIN )	{
				if( soc->send_unacked == soc->send_next) {
					soc->event_listener(sock, TCP_EVENT_CLOSE, soc->rem_ip, soc->remport);
					soc->receive_next++;
					soc->send_next++;
					soc->myflags = TCP_FLAG_ACK | TCP_FLAG_FIN;
					tcp_newstate(sock, TCP_STATE_LAST_ACK);
					tcp_sendcontrol(sock);
					return 0;
				}
			}
			if(dlen) soc->ack_timer = 1;
			tcp_newstate(sock, TCP_STATE_CONNECTED);			
			return 0;
		case TCP_STATE_FREE:
		case TCP_STATE_CLOSED:
			tcp_sendreset(tcphdr, iphdr->srcip);
			return -1;
		case TCP_STATE_LISTENING:
			if(tcphdr->hlen_flags & TCP_FLAG_RESET) {
				tcp_newstate(sock, TCP_STATE_LISTENING);
				return -1;
			}
			if(tcphdr->hlen_flags & TCP_FLAG_ACK) {
				tcp_newstate(sock, TCP_STATE_LISTENING);
				tcp_sendreset(tcphdr, iphdr->srcip);
				return -1;
			}
			if((tcphdr->hlen_flags & TCP_FLAG_SYN) == 0) {
				tcp_newstate(sock, TCP_STATE_LISTENING);
				tcp_sendreset(tcphdr, iphdr->srcip);
				return -1;
			}
			temp = (int16_t)soc->event_listener(sock, TCP_EVENT_CONREQ, soc->rem_ip, soc->remport);
			if(temp == -1) {
				tcp_sendreset(tcphdr, iphdr->srcip);
				return -1;
			}
			if(temp == -2) return 1;
			if(soc->flags & TCP_INTFLAGS_CLOSEPENDING) soc->flags ^= TCP_INTFLAGS_CLOSEPENDING;
			tcp_newstate(sock, TCP_STATE_SYN_RECEIVED);
			soc->receive_next = tcphdr->seqno + 1;	/* Ack SYN */
			soc->send_unacked = tcp_initseq();
			soc->myflags = TCP_FLAG_SYN | TCP_FLAG_ACK;
			tcp_sendcontrol(sock);
			soc->send_next = soc->send_unacked + 1;
			return 1;
		case TCP_STATE_SYN_RECEIVED:
			if(tcphdr->hlen_flags & TCP_FLAG_RESET)	{
				soc->event_listener(sock, TCP_EVENT_ABORT, soc->rem_ip, soc->remport);
				if(soc->type & TCP_TYPE_SERVER)
					tcp_newstate(sock, TCP_STATE_LISTENING);
				else
					tcp_newstate(sock, TCP_STATE_CLOSED);
				return -1;
			}
			/* Is it SYN+ACK (if we are the because of simultaneous open)	*/
			if((tcphdr->hlen_flags & TCP_FLAG_SYN) &&
			   (tcphdr->hlen_flags & TCP_FLAG_ACK)) {			
				if( tcphdr->ackno != soc->send_next ) return -1;
				soc->receive_next =  tcphdr->seqno;
				soc->receive_next++;	/* ACK SYN */
				soc->send_unacked = soc->send_next;
				tcp_newstate(sock, TCP_STATE_CONNECTED);
				soc->myflags = TCP_FLAG_ACK;
				tcp_sendcontrol(sock);
				soc->event_listener(sock, TCP_EVENT_CONNECTED, soc->rem_ip, soc->remport);
				return 0;
			}
			/* Is it ACK? */
			if( tcphdr->hlen_flags & TCP_FLAG_ACK )	{
				if(tcphdr->ackno != soc->send_next) return -1;
				if(tcphdr->seqno != soc->receive_next) return -1;
				soc->send_unacked = soc->send_next;
				tcp_newstate(sock, TCP_STATE_CONNECTED);
				soc->event_listener(sock, TCP_EVENT_CONNECTED, soc->rem_ip, soc->remport);
				return 0;
			}
			if(tcphdr->hlen_flags & TCP_FLAG_SYN) return 0;
			tcp_sendreset(tcphdr, iphdr->srcip);
			return -1;
		case TCP_STATE_SYN_SENT:
			if(tcphdr->hlen_flags & TCP_FLAG_RESET) {
				soc->event_listener(sock, TCP_EVENT_ABORT, soc->rem_ip, soc->remport);
				if(soc->type & TCP_TYPE_SERVER)
					tcp_newstate(sock, TCP_STATE_LISTENING);
				else
					tcp_newstate(sock, TCP_STATE_CLOSED);
				return -1;
			}
			/* Is it SYN+ACK? */
			if((tcphdr->hlen_flags & TCP_FLAG_SYN) &&
			   (tcphdr->hlen_flags & TCP_FLAG_ACK)) {
				/* Right ACK?	*/
				if( tcphdr->ackno != soc->send_next ) return -1;
				soc->receive_next = tcphdr->seqno;
				soc->receive_next++;	/* ACK SYN */
				soc->send_unacked = soc->send_next;
				tcp_newstate(sock, TCP_STATE_CONNECTED);
				soc->myflags = TCP_FLAG_ACK;
				tcp_sendcontrol(sock);
				soc->event_listener(sock, TCP_EVENT_CONNECTED, soc->rem_ip, soc->remport);
				return 0;
			}
			/* Is it SYN (simultaneous open) */
			if(tcphdr->hlen_flags & TCP_FLAG_SYN) {
				soc->receive_next =  tcphdr->seqno;
				soc->receive_next++; /* ACK SYN	*/				
				tcp_newstate(sock, TCP_STATE_SYN_RECEIVED);
				soc->myflags = TCP_FLAG_SYN | TCP_FLAG_ACK;
				tcp_sendcontrol(sock);
				return 0;
			}			
			tcp_sendreset(tcphdr, iphdr->srcip);
			return -1;
		case TCP_STATE_FINW1:
			if(tcphdr->hlen_flags & TCP_FLAG_RESET) {
				soc->event_listener(sock, TCP_EVENT_ABORT, soc->rem_ip, soc->remport);
				if(soc->type & TCP_TYPE_SERVER)
					tcp_newstate(sock, TCP_STATE_LISTENING);
				else
					tcp_newstate(sock, TCP_STATE_CLOSED);
				return -1;
			}
			/* Is it FIN+ACK? */
			if((tcphdr->hlen_flags & TCP_FLAG_FIN) &&
			   (tcphdr->hlen_flags & TCP_FLAG_ACK)) {
				if(tcphdr->ackno != soc->send_next) return -1;
				soc->receive_next = tcphdr->seqno;
				soc->receive_next++;
				soc->receive_next += dlen;
				soc->send_unacked = soc->send_next;
				tcp_newstate(sock, TCP_STATE_TIMED_WAIT);
				soc->myflags = TCP_FLAG_ACK;
				tcp_sendcontrol(sock);
				return 0;
			}
			/* Is it just FIN */
			if(tcphdr->hlen_flags & TCP_FLAG_FIN) {
				soc->receive_next = tcphdr->seqno;
				soc->receive_next++;
				soc->receive_next += dlen;
				tcp_newstate(sock, TCP_STATE_CLOSING);
				soc->myflags = TCP_FLAG_ACK;
				tcp_sendcontrol(sock);			
				return 0;
			}			
			/* Is it just ACK? */
			if(tcphdr->hlen_flags & TCP_FLAG_ACK) {
				/* Right ACK?	*/
				if( tcphdr->ackno != soc->send_next ) return -1;
				soc->send_unacked = soc->send_next;
				tcp_newstate(sock, TCP_STATE_FINW2);
				return 0;
			}
			break;
		case TCP_STATE_FINW2:
			if(tcphdr->hlen_flags & TCP_FLAG_RESET) {
				soc->event_listener(sock, TCP_EVENT_ABORT, soc->rem_ip, soc->remport);
				if(soc->type & TCP_TYPE_SERVER)
					tcp_newstate(sock, TCP_STATE_LISTENING);
				else
					tcp_newstate(sock, TCP_STATE_CLOSED);
				return -1;
			}
			if(tcphdr->hlen_flags & TCP_FLAG_FIN) {
				soc->receive_next = tcphdr->seqno;
				soc->receive_next++;
				soc->receive_next += dlen;
				tcp_newstate(sock, TCP_STATE_TIMED_WAIT);
				soc->myflags = TCP_FLAG_ACK;
				tcp_sendcontrol(sock);			
				return 0;
			}		
			break;
		case TCP_STATE_CLOSING:
			if(tcphdr->hlen_flags & TCP_FLAG_RESET)	{
				soc->event_listener(sock, TCP_EVENT_ABORT, soc->rem_ip, soc->remport);
				if(soc->type & TCP_TYPE_SERVER)
					tcp_newstate(sock, TCP_STATE_LISTENING);
				else
					tcp_newstate(sock, TCP_STATE_CLOSED);
				return -1;
			}
			/* Is it ACK? */
			if( tcphdr->hlen_flags & TCP_FLAG_ACK	) {
				/* Right ACK? */
				if( tcphdr->ackno != soc->send_next ) return -1;
				soc->send_unacked = soc->send_next;
				tcp_newstate(sock, TCP_STATE_TIMED_WAIT);
				return 0;
			}
			if(tcphdr->hlen_flags & TCP_FLAG_FIN) {
				soc->receive_next = tcphdr->seqno;
				soc->receive_next++;
				soc->receive_next += dlen;
				soc->myflags = TCP_FLAG_ACK;
				tcp_sendcontrol(sock);			
				return 0;
			}
			break;
		case TCP_STATE_LAST_ACK:
			if(tcphdr->hlen_flags & TCP_FLAG_RESET) {
				soc->event_listener(sock, TCP_EVENT_ABORT, soc->rem_ip, soc->remport);
				if(soc->type & TCP_TYPE_SERVER)
					tcp_newstate(sock, TCP_STATE_LISTENING);
				else
					tcp_newstate(sock, TCP_STATE_CLOSED);
				return -1;
			}
			/* Is it ACK?	*/
			if( tcphdr->hlen_flags & TCP_FLAG_ACK	) {
				/* Right ACK? */
				if(tcphdr->ackno != soc->send_next) return -1;
				soc->send_unacked = soc->send_next;				
				if(soc->type & TCP_TYPE_SERVER)
					tcp_newstate(sock, TCP_STATE_LISTENING);
				else
					tcp_newstate(sock, TCP_STATE_CLOSED);
				return 0;
			}			
			/* Is it repeated FIN?	*/
			if(tcphdr->hlen_flags & TCP_FLAG_FIN) {
				/* ACK FIN and all data	*/
				soc->receive_next = tcphdr->seqno;
				soc->receive_next++;
				soc->receive_next += dlen;
				soc->myflags = TCP_FLAG_FIN | TCP_FLAG_ACK;
				tcp_sendcontrol(sock);
				return 0;
			}			
			break;
		case TCP_STATE_TIMED_WAIT:
			if(tcphdr->hlen_flags & TCP_FLAG_RESET) {
				soc->event_listener(sock, TCP_EVENT_ABORT, soc->rem_ip, soc->remport);
				if(soc->type & TCP_TYPE_SERVER)
					tcp_newstate(sock, TCP_STATE_LISTENING);
				else
					tcp_newstate(sock, TCP_STATE_CLOSED);
				return -1;
			}
			/* Is it repeated FIN?	*/
			if(tcphdr->hlen_flags & TCP_FLAG_FIN) {
				soc->receive_next = tcphdr->seqno;
				soc->receive_next++;
				soc->receive_next += dlen;
				soc->myflags = TCP_FLAG_ACK;
				tcp_sendcontrol(sock);
				return 0;
			}
			break;
		default:
			tcp_sendreset(tcphdr, iphdr->srcip);
			return -1;
	}
	return -1;
}

void tcp_interval() {
	int32_t	 i;
	Task	 *task;
	tcp_info *soc;

	for(i = 0;i < TCP_NUM;i++) {
		soc = &tcp_socket[i];
		task = get_taskptr(soc->id);
		switch(tcp_socket[i].state) {
		case TCP_STATE_CONNECTED:
			if(soc->resend_timer > 0) {
				if(--(soc->resend_timer) == 0) {
					task->state &= ~NET_STATE;
					free_mem(meminfo, SuperPID, soc->packet);
					tcp_abort(i);
					tcp_releasesocket(task->pid, i);
					return;
				} else {
					tcp_out(i);
				}
			}
			if(soc->ack_timer > 0) {
				if(--(soc->ack_timer) == 0) {
					soc->myflags = TCP_FLAG_ACK;
					tcp_sendcontrol(i);
				}
			}
			break;
		case TCP_STATE_TIMED_WAIT:
			if((soc->close_count) > 0) {
				if(--(soc->close_count) == 0) {
					if(soc->type & TCP_TYPE_SERVER)
						tcp_newstate(i, TCP_STATE_LISTENING);
					else
						tcp_newstate(i, TCP_STATE_CLOSED);
				}
			}
			break;
		}
		if((soc->fin_timer) > 0) {
			if(--(soc->fin_timer) == 0) {
				task->state &= ~NET_STATE;
				task->retval = -1;
				tcp_abort(i);
				tcp_releasesocket(soc->id, i);
			}
		}
	}
}

static int32_t tcp_listener(uint32_t sock, Byte flag, uint32_t rip, uint16_t rport) {
	tcp_info *soc, *newsoc;
	Task	*taskptr;
	struct sockaddr *client;
	STRING	tcpdata, data;
	int32_t	datasize, size, ret;
	int32_t newsock;
	int32_t isize;
	STRING	buf;

	ret = 0;
	soc = &tcp_socket[sock];
	taskptr = get_taskptr(soc->id);
	if(taskptr == 0) return -1;
	switch(flag) {
		case TCP_EVENT_CONREQ:
			ret = -1;
			if(soc->type & TCP_TYPE_SERVER &&
			   taskptr->req == TCP_ACCEPT_REQ &&
			   taskptr->state & NET_STATE &&
			   sock == (taskptr->arg[0] & SOCK_MASK)) {
				ret = 0;
			}
			break;
		case TCP_EVENT_CONNECTED:
			if(soc->type & TCP_TYPE_SERVER &&
			   taskptr->req == TCP_ACCEPT_REQ &&
			   taskptr->state & NET_STATE &&
			   sock == (taskptr->arg[0] & SOCK_MASK)) {
				taskptr->state &= ~NET_STATE;
				client = (struct sockaddr*)taskptr->arg[1];
				client->sin_addr = rip;
				client->sin_port = rport;
				newsock = tcp_dupsocket(sock);
				soc->rem_ip = 0;
				soc->remport = 0;
				if(newsock == -1) {
					taskptr->retval = -1;
				} else {
					newsoc = &tcp_socket[newsock];
					newsoc->type = TCP_TYPE_CLIENT;
					taskptr->retval = newsock | TCP_SOCK;
				}
				tcp_newstate(sock, TCP_STATE_LISTENING);
			} else newsoc = soc;
			InitTCPSock(newsoc,newsoc->receive_next); // Add T.Mochizuki
			break;
		case TCP_EVENT_DATA:
			datasize = rport;
			tcpdata = (STRING)rip;
			if(soc->recv_packet_size > 0) free_mem(meminfo, SuperPID, soc->recv_packet);
			soc->recv_packet_size = datasize;
			soc->recv_packet = alloc_mem(meminfo, SuperPID, datasize);
			memcpy(soc->recv_packet, tcpdata, datasize);
			ret = SetTCPRData(soc,tcpdata,datasize);
			break;
		case TCP_EVENT_ACK:
			break;
		case TCP_EVENT_CLOSE:
		case TCP_EVENT_ABORT:
			if(taskptr->req == TCP_RECV_REQ &&
			   taskptr->state & NET_STATE &&
			   sock == (taskptr->arg[0] & SOCK_MASK)) {
				taskptr->state &= ~NET_STATE;
			}
			break;
		default:
			ret = -1;
	}
	return ret;
}

int32_t	tcpsocket(int16_t id) {
	int32_t sock;

	sock = tcp_getsocket(id, tcp_listener);
	if(sock != -1) sock |= TCP_SOCK;
	return sock;
}

int32_t tcpfree(int16_t id, int32_t s) {
	Task	*task;
	tcp_info *soc;
	int32_t	ret, sock;

	sock = s & SOCK_MASK;
	if(sock >= TCP_NUM) return -1;
	soc = &tcp_socket[sock];
	tcp_close(id, sock);
	ret = tcp_releasesocket(id, sock);
	if(ret == -1) {
		task = get_taskptr(id);
		task->state |= NET_STATE;
		soc->fin_timer = 2;
	}
	return ret;
}

int32_t tcpfreeid(int16_t id) {
	int32_t	sock;

	for(sock = 0;sock < TCP_NUM;sock++) {
		if(tcp_socket[sock].id == id) {
			tcp_abort(sock);
			tcp_releasesocket(id, sock);
		}
	}
	return 0;
}

int32_t tcplisten(int16_t id, int32_t sock, struct sockaddr *addr) {
	return tcp_listen(id, sock & SOCK_MASK, addr->sin_port);
}
int32_t tcpgetdstIP(uint32_t s,uint32_t *ip)
{
  
  Task	 *task;
  tcp_info *soc;
  int32_t	 sock;

  sock = s & SOCK_MASK;
  if(sock >= TCP_NUM) return -1;
  soc = &tcp_socket[sock];
  if(soc->state != TCP_STATE_CONNECTED) return -1;
  *ip = soc->rem_ip;
  return 0;
}

int32_t tcpsend(int16_t id, uint32_t s, STRING data, int32_t size) {
	Task	 *task;
	tcp_info *soc;
	int32_t	 sock;

	sock = s & SOCK_MASK;
	if(sock >= TCP_NUM) return -1;
	soc = &tcp_socket[sock];
	if(soc->state != TCP_STATE_CONNECTED) return -1;
	if(size + MIN_TCP_HLEN > soc->send_mtu) return -1;
	soc->id = id;
	if(size == 0) return 0;
	if(soc->resend_timer > 0) {
		soc->next_packet = data;
		soc->next_packet_size = size;
		task = get_taskptr(soc->id);
		task->state |= NET_STATE;
		return 0;
	}
	if(soc->send_unacked != soc->send_next) return -1;
	soc->packet_size = size;
	soc->packet = alloc_mem(meminfo, SuperPID, soc->packet_size);
	memcpy(soc->packet, data, size);
	soc->send_next += soc->packet_size;
	soc->resend_timer = TCP_DEF_RETRIES;
	tcp_out(sock);
	return size;
}

int32_t tcprecv(int16_t id, uint32_t s, STRING data, int32_t size) {
	Task	 *task;
	tcp_info *soc;
	int32_t	 sock, datasize, ret;

	sock = s & SOCK_MASK;
	if(sock >= TCP_NUM) return -1;
	soc = &tcp_socket[sock];
	if(soc->state != TCP_STATE_CONNECTED) return -1;
	soc->id = id;
	if(size == 0) return 0;
	task = get_taskptr(soc->id);
	ret = ReadTCPRData(soc,data,size); 
	if(soc->recv_packet_size > 0) {
	  free_mem(meminfo, SuperPID, soc->recv_packet);
	  soc->recv_packet_size = 0;
	}
	return ret;
}
