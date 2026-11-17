import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

@main
def main(): Unit =
  val x = 1
  val r = if x > 0 then tr("t", 1) else tr("e", 2)
  print("|")
  print(r)
