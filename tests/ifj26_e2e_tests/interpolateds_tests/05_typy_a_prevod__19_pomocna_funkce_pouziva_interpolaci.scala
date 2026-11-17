import ifj26.*

def label(n: Int, name: String): String = s"$name#$n"

@main
def main(): Unit =
  val a = label(1, "x")
  val b = label(22, "y")
  print(a)
  print(b)
