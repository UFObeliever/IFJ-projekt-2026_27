import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

@main
def main(): Unit =
  if tr("x", 1) < tr("y", 2) then
    print("|yes")
  else
    print("|no")
