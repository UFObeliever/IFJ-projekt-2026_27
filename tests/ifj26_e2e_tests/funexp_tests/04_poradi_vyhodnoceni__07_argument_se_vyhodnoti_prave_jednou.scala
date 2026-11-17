import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

def sq(x: Int): Int = x * x

@main
def main(): Unit =
  val r = sq(tr("a", 3))
  print("|" + toStr(r))
