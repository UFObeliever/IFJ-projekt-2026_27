import ifj26.*

def flip(b: Boolean): Boolean =
  var r = b
  r = !r
  r

@main
def main(): Unit =
  val a = flip(true)
  val b = flip(false)
  print(a)
  print(b)
