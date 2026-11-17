import ifj26.*

extension (v: Int)
  def twice: Int = v * 2

@main
def main(): Unit =
  var x = 1
  x = x.twice
  x = x.twice
  x = x.twice
  print(x)
