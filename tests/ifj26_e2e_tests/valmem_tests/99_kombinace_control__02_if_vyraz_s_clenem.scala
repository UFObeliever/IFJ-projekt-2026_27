import ifj26.*

@main
def main(): Unit =
  val s = "abc"
  val m = if s.length > 2 then s.length else 0
  print(m)
