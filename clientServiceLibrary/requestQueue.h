#ifndef _REQUESTQUEUE_H
#define _REQUESTQUEUE_H

#include "clientRequestImpl.h"

namespace pluton {

  class requestQueue {
  public:
    requestQueue(const char* name);
    ~requestQueue();

    void	addToHead(pluton::clientRequestImpl* R);
    bool	deleteRequest(pluton::clientRequestImpl* R);
    bool	empty() const { return _headOfQueue ? false : true; }

    pluton::clientRequestImpl*	getFirst() const { return _headOfQueue; }
    pluton::clientRequestImpl*	getNext(const pluton::clientRequestImpl* R) const
      { return R->getNext(); }

    unsigned int		count() const { return _count; }
    void			dump(const char* who) const;

  private:
    const char*			_name;
    pluton::clientRequestImpl*	_headOfQueue;
    unsigned int		_count;
  };
}

#endif
