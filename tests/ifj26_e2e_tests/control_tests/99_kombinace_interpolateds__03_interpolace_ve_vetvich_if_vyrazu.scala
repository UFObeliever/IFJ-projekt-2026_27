import ifj26.*

@main
def main(): Unit =
  val n = 3
  val m = if n > 2 then s"big $n" else s"small $n"
  print(m)
