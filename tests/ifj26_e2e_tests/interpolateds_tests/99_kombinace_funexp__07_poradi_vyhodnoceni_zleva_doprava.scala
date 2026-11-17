import ifj26.*

def pa(v: Int): Int =
  print("a")
  v

def pb(v: Int): Int =
  print("b")
  v

def pc(v: Int): Int =
  print("c")
  v

@main
def main(): Unit =
  val m = s"${pa(1)}${pb(2)}${pc(3)}"
  print("|")
  print(m)
