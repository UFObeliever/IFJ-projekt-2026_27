import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

def f(a: Int)(b: Int)(c: Int): Int = a * 100 + b * 10 + c

@main
def main(): Unit =
  val r = f(tr("a", 1))(tr("b", 2))(tr("c", 3))
  print("|" + toStr(r))
