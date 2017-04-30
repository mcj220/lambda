-- utility
def identity x = x
def cond e1 e2 c = builtin_cond e1 e2 c

-- selectors
def make_pair first second = builtin_make_pair first second
def select_first first second = builtin_select_first first second
def select_second first second = builtin_select_second first second

-- boolean truth values
def true = builtin_true
def false = builtin_false

-- boolean functions
def not s = ((s false) true)
def and a b = ((a b) false)
def or a b = ((a true) b)
def implies a b = ((a b) true)
def equiv a b = ((a b) (not b))

-- numeric functions
def iszero n = builtin_iszero n
def succ n = builtin_succ n
def pred n = builtin_pred n
def add x y = builtin_add x y

rec mult x y = \
	if iszero y \
	then 0 \
	else add x mult x pred y

rec power x y = \
	if iszero y \
	then 1 \
	else mult x power x pred y

def sub x y = builtin_sub x y

def abs_diff x y = builtin_abs_diff x y
def equal x y = builtin_equal x y
def greater x y = not iszero sub x y
def greater_or_eq x y = iszero sub y x
def less x y = not iszero sub y x
def less_or_eq x y = iszero sub x y

rec div1 x y = \
	if greater y x \
	then 0 \
	else succ div1 sub x y y

def div x y = \
	if iszero y \
	then 0 \
	else div1 x y

rec mod1 x y = \
	if less x y \
	then x \
	else mod1 sub x y y

def mod x y = \
	if iszero y \
	then 0 \
	else mod1 x y

rec factorial n = \
	if iszero n \
	then 1 \
	else mult n factorial pred n

def sq x = mult x x
def double x = mult 2 x

-- types
def make_obj t v = builtin_make_obj t v
def type obj = builtin_type obj
def value obj = builtin_value obj
def istype t obj = builtin_istype t obj

def error_type = builtin_error_type
def MAKE_ERROR = (make_obj error_type)
def ERROR = (MAKE_ERROR error_type)
def iserror x = istype error_type x

def bool_type = 1
def MAKE_BOOL x = ((make_obj bool_type) x)
def TRUE = (MAKE_BOOL true)
def FALSE = (MAKE_BOOL false)
def isbool x = builtin_isbool x
def BOOL_ERROR = builtin_bool_error
def NOT X = \
	if isbool X \
	then (MAKE_BOOL not value X) \
	else BOOL_ERROR

def AND X Y = \
	if and isbool X isbool Y \
	then MAKE_BOOL ((and (value X)) (value Y)) \
	else BOOL_ERROR
