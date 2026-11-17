import ifj26.*

extension (v: Int)
  def twice: Int = v * 2

@main
def main(): Unit =
  print(toStr(readInt().twice))
