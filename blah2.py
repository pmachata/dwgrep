class dwgrep_magic (object):
    pass

def surrogatize (that):
    if isinstance (that, dwgrep_magic):
        return that

    def surrogate (X):
        class surrogate (traversal):
            def gen (self, v):
                yield X
        return surrogate ()

    return surrogate (that)

class traversal (dwgrep_magic):
    def build_cmp (self, that, F):
        that = surrogatize (that)
        this = self
        class Cmp (selector):
            def match (self, v):
                return F (set (this.gen (v)), set (that.gen (v)))

        return Cmp ()

    def __eq__ (self, that):
        F = lambda ours, theirs: len (ours.intersection (theirs)) > 0
        return self.build_cmp (that, F)

    def __ne__ (self, that):
        def F (ours, theirs):
            if len (ours) > 1 or len (theirs) > 1:
                return True
            return ours.pop () != theirs.pop ()

        return self.build_cmp (that, F)

    def build_arith (self, that, A):
        that = surrogatize (that)
        this = self
        class Arith (traversal):
            def gen (self, v):
                for a in this.gen (v):
                    for b in that.gen (v):
                        yield A (a, b)

        return Arith ()

    def __add__ (self, that):
        return self.build_arith (that, lambda a, b: a + b)

    def __sub__ (self, that):
        return self.build_arith (that, lambda a, b: a - b)

    def __mul__ (self, that):
        return self.build_arith (that, lambda a, b: a * b)

    def __div__ (self, that):
        return self.build_arith (that, lambda a, b: a / b)

    def __mod__ (self, that):
        return self.build_arith (that, lambda a, b: a % b)

    def __getitem__ (self, sel):
        this = self
        class Filter (traversal):
            def gen (self, v):
                for i in this.gen (v):
                    if sel.match (i):
                        yield i
        return Filter ()

    def __call__ (self, *args):
        this = self
        class Traverse (traversal):
            def gen (self, v):
                for i in this.gen (v):
                    for trav in args:
                        trav = surrogatize (trav)
                        for v2 in trav.gen (i):
                            yield v2
        return Traverse ()

    def __iter__ (self):
        for v in self.gen (None):
            yield v

    def dump (self):
        for i in self:
            print i,
        print

class selector (dwgrep_magic):
    def __or__ (self, other):
        class or_sel (selector):
            def __init__ (self, a, b):
                self._a = a
                self._b = b
            def match (self, obj):
                return self._a.match (obj) or self._b.match (obj)

        return or_sel (self, other)

    def __invert__ (self):
        class not_sel (selector):
            def __init__ (self, a):
                self._a = a
            def match (self, obj):
                return not self._a.match (obj)

        return not_sel (self)

class Q (traversal):
    def gen (self, v):
        yield None

Q = Q ()

class winfo (traversal):
    def gen (self, v):
        yield 1
        yield 2
        yield 3
        yield "hello"
        yield "world"

winfo = winfo ()

class elem (traversal):
    def gen (self, v):
        try:
            g = iter (v)
        except TypeError:
            return
        except:
            raise

        for i in g:
            yield i

elem = elem ()

class _ (traversal):
    def gen (self, v):
        yield v

_ = _ ()

class type (traversal):
    def gen (self, v):
        import __builtin__
        yield __builtin__.type (v)

type = type ()

def list (*args):
    class List (traversal):
        def gen (self, v):
            import __builtin__
            ret = []
            for trav in args:
                trav = surrogatize (trav)
                ret += __builtin__.list (trav.gen (v))
            yield ret
    return List ()

Q (winfo) .dump ()
Q (winfo) [~(elem == 'd')] (elem) [_ == 'l'] .dump ()
Q (winfo) [_ != "hello"] .dump ()
Q (winfo) (elem + elem) [~(elem != elem)] .dump ()
Q (winfo) [type == str] .dump ()
Q (winfo) (elem [_ == 'l'] + _ + elem [_ != 'l']) .dump ()
Q (winfo[type == int] + winfo[type == int]) [_ % 2 == 0] .dump ()
Q (list (winfo)) .dump ()
Q (winfo) (list (elem)) .dump ()
Q (list (*range (10))) (elem) .dump ()
