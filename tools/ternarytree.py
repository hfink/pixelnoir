# Copyright (c) 2010 Heinrich Fink <hf (at) hfink (dot) eu>, 
#                    Thomas Weber <weber (dot) t (at) gmx (dot) at>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

class TernaryTree:

    def __init__(self,
                 cd1,
                 cd2,
                 ca,
                 clr,
                 cvr,
                 T,
                 ts
                 ):
        self.dim_scale = 0.01
        self._cd1 = cd1
        self._cd2 = cd2
        self._ca = ca
        self._clr = clr
        self._cvr = cvr
        self.T = T
        self.ts = ts
        self._p = {}

        self._p["A"] = self._A
        self._p["F"] = self._F
        self._p["!"] = self._excl

    def produce(self, num_iterations):
        self._max_depth = num_iterations
        production = self._p["!"](1,0) + self._p["F"](200,0) + "/" + \
                     "(45)"+self._p["A"](0)
        return production
        
    def _A(self, n):

        # if we are already at the maximum depth, an apex would not create anything
        if n >= self._max_depth:
            return ""        

        n_i = n+1
        return self._excl(self._cvr,n_i)+self._F(50,n_i)+"[&("+str(self._ca)+")"+self._F(50,n_i)+self._A(n_i)+"]/("\
               +str(self._cd1)+")[&("+str(self._ca)+")"+self._F(50,n_i)+self._A(n_i)+"]/("+str(self._cd2)\
               +")[&("+str(self._ca)+")"+self._F(50,n_i)+self._A(n_i)+"]"

    def _F(self, l, n):

        def str_rep():
            return "F("+str(l)+")"
        
        if n >= self._max_depth:
            return str_rep()

        return self._F(l*self._clr,n+1)

    def _excl(self, w,n):
        def str_rep():
            return "!("+str(w)+")"
        
        if n >= self._max_depth:
            return str_rep()

        return self._excl(w*self._cvr,n+1)






